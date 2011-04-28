/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

/*
 * Flyby routines.
 *
 * Record or playback previously recorded actions.
 * We try to record everything the user does, except
 * for some things which would be annoying (such as
 * certain options or toggling of certain panels).
 */

#include "mon.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "panel.h"
#include "pathnames.h"
#include "queue.h"
#include "reel.h"
#include "selnode.h"
#include "state.h"
#include "xmath.h"

/* Flyby record types (used in flyby record headers). */
#define FHT_INIT	1
#define FHT_SEQ		2
#define FHT_SELNODE	3
#define FHT_PANEL	4
#define FHT_HLNC	5
#define FHT_CAPTION	6

struct fbhdr {
	int		fbh_type;
	size_t		fbh_len;
};

struct fbinit {
	struct state	fbi_state;
	int		fbi_panels;
};

struct fbpanel {
	int		fbp_panel;
};

struct fbselnode {
	int		fbsn_nid;
	struct fvec	fbsn_offv;
};

struct fbhlnc {
	int		fbhl_nc;
	int		fbhl_op;
};

struct fbcaption {
	char		fbcap_caption[BUFSIZ];
};

union fbun {
	struct fbinit		fbu_init;
	struct fbpanel		fbu_panel;
	struct fbselnode	fbu_sn;
	struct fbhlnc		fbu_hlnc;
	struct fbcaption	fbu_cap;
};

int		 flyby_mode = FBM_OFF;
int		 flyby_autoto = 2 * 60 * 60;	/* time till autoflyby turns on */
int		 flyby_nautoto;			/* see flyby_rstautoto */
int		 flyby_len;
int		 flyby_pos;
static FILE	*flyby_fp;
struct state	 sav_st;			/* preserve old state during flyby */
struct objlist	 flyby_list = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };
char		 flyby_fn[PATH_MAX] = _PATH_FLYBYDIR "/" FLYBY_DEFAULT;
char		 flyby_dir[PATH_MAX] = _PATH_FLYBYDIR;

void
flyby_calclen(void)
{
	struct fbhdr fbh;

	flyby_len = 0;

	while (fread(&fbh, 1, sizeof(fbh), flyby_fp) == sizeof(fbh)) {
		switch (fbh.fbh_type) {
		case FHT_INIT:
		case FHT_SEQ:
			flyby_len++;
			break;
		}
		if (fseek(flyby_fp, fbh.fbh_len, SEEK_CUR) == -1)
			err(1, "flyby fseek");
	}
	if (ferror(flyby_fp))
		err(1, "fread flyby header");
	rewind(flyby_fp);
}

void
flyby_writemsg(int type, const void *data, size_t len)
{
	struct fbhdr fbh;

	fbh.fbh_type = type;
	fbh.fbh_len = len;
	if (fwrite(&fbh, 1, sizeof(fbh), flyby_fp) != sizeof(fbh))
		err(1, "fwrite flyby header");
	if (fwrite(data, 1, len, flyby_fp) != len)
		err(1, "fwrite flyby data");
}

/* Write current data for flyby. */
void
flyby_writeseq(struct state *stp)
{
	flyby_writemsg(FHT_SEQ, stp, sizeof(*stp));
}

void
flyby_writeinit(struct state *stp)
{
	struct fbinit fbi;
	struct panel *p;

	/* One copy per init msg should be okay. */
	fbi.fbi_state = *stp;
	fbi.fbi_panels = 0;

	TAILQ_FOREACH(p, &panels, p_link)
		fbi.fbi_panels |= p->p_id;

	flyby_writemsg(FHT_INIT, &fbi, sizeof(fbi));
}

__inline void
flyby_writepanel(int panel)
{
	flyby_writemsg(FHT_PANEL, &panel, sizeof(panel));
}

__inline void
flyby_writeselnode(int nid, const struct fvec *offv)
{
	struct fbselnode fbsn;

	fbsn.fbsn_nid = nid;
	fbsn.fbsn_offv = *offv;
	flyby_writemsg(FHT_SELNODE, &fbsn, sizeof(fbsn));
}

__inline void
flyby_writehlnc(int nc, int op)
{
	struct fbhlnc fbhl;

	fbhl.fbhl_nc = nc;
	fbhl.fbhl_op = op;
	flyby_writemsg(FHT_HLNC, &fbhl, sizeof(fbhl));
}

__inline void
flyby_writecaption(const char *cap)
{
	struct fbcaption fbcap;

	snprintf(fbcap.fbcap_caption, sizeof(fbcap.fbcap_caption),
	    "%s", cap);
	flyby_writemsg(FHT_CAPTION, &fbcap, sizeof(fbcap));
}

/* Open the flyby data file appropriately. */
void
flyby_begin(int mode)
{
	struct selnode *sn;

	if (flyby_fp != NULL)
		return;

	switch (mode) {
	case FBM_PLAY:
		if ((flyby_fp = fopen(flyby_fn, "rb")) == NULL) {
			if (errno == ENOENT)
				return;
			err(1, "%s", flyby_fn);
		}
		flyby_mode = FBM_PLAY;

		/*
		 * Backup current options and enable/disable
		 * special options for the flyby duration.
		 */
		sav_st = st;
		st.st_opts &= ~OP_TWEEN;
//		opt_disable(OP_TWEEN);		/* save tween */

//		panel_show(PANEL_FLYBY);
		if (st.st_opts & OP_REEL) {
//			panel_show(PANEL_REEL);
			flyby_calclen();
			flyby_pos = 0;
			reel_start();
		}

		glutMotionFunc(gl_motionh_null);
		glutPassiveMotionFunc(gl_pasvmotionh_null);
		glutKeyboardFunc(gl_keyh_actflyby);
		glutSpecialFunc(gl_spkeyh_actflyby);
		glutMouseFunc(gl_mouseh_null);
		cursor_set(GLUT_CURSOR_WAIT);
		sn_clear();
		break;
	case FBM_REC:
		if ((flyby_fp = fopen(flyby_fn, "ab")) == NULL)
			err(1, "%s", flyby_fn);
		flyby_mode = FBM_REC;
		flyby_writeinit(&st);
		SLIST_FOREACH(sn, &selnodes, sn_next)
			flyby_writeselnode(sn->sn_nodep->n_nid, &sn->sn_offv);
		break;
	}
}

/* Read a set of flyby data. */
void
flyby_read(void)
{
	int i, done, oldrf, oldopts, optdiff;
	void (*f)(struct fill *);
	struct fbhdr fbh;
	union fbun fbun;
	struct node *n;

	oldopts = st.st_opts;
	oldrf = st.st_rf;

	done = 0;
	do {
		if (fread(&fbh, 1, sizeof(fbh), flyby_fp) != sizeof(fbh)) {
			if (feof(flyby_fp)) {
				if (st.st_opts & OP_LOOPFLYBY) {
					rewind(flyby_fp);
					flyby_pos = 0;
					reel_pos = -1;
					continue;
				} else {
					flyby_end();
					return;
				}
			}
			err(1, "fread flyby header");
		}
		switch (fbh.fbh_type) {
		case FHT_INIT:
			if (fread(&fbun, 1, sizeof(struct fbinit),
			    flyby_fp) != sizeof(struct fbinit))
				err(1, "flyby read init");
			st = fbun.fbu_init.fbi_state;
			st.st_rf |= RF_VMODE | RF_DMODE | RF_CAM |
			    RF_SELNODE;
			panels_set(fbun.fbu_init.fbi_panels);
//		egg_toggle(st.st_eggs ^ sav_st.st_eggs);
			done = 1;
			break;
		case FHT_SEQ:
			if (fread(&st, 1, sizeof(st), flyby_fp) !=
			    sizeof(st))
				err(1, "flyby read seq");
			st.st_rf |= RF_CAM;
			done = 1;
			break;
		case FHT_PANEL:
			if (fread(&fbun, 1, sizeof(struct fbpanel),
			    flyby_fp) != sizeof(struct fbpanel))
				err(1, "flyby read panel");
			if (fbun.fbu_panel.fbp_panel >= 0 &&
			    fbun.fbu_panel.fbp_panel < NPANELS)
				panel_toggle(fbun.fbu_panel.fbp_panel);
			break;
		case FHT_SELNODE:
			if (fread(&fbun, 1, sizeof(struct fbselnode),
			    flyby_fp) != sizeof(struct fbselnode))
				err(1, "flyby read selnode");
			if ((n = node_for_nid(fbun.fbu_sn.fbsn_nid)) != NULL)
				sn_toggle(n, &fbun.fbu_sn.fbsn_offv);
			break;
		case FHT_HLNC:
			if (fread(&fbun, 1, sizeof(struct fbhlnc),
			    flyby_fp) != sizeof(struct fbhlnc))
				err(1, "flyby read hlnc");
			f = NULL;
			switch (fbun.fbu_hlnc.fbhl_op) {
			case FBHLOP_XPARENT:
				f = fill_setxparent;
				break;
			case FBHLOP_OPAQUE:
				f = fill_setopaque;
				break;
			case FBHLOP_ALPHAINC:
				f = fill_alphainc;
				break;
			case FBHLOP_ALPHADEC:
				f = fill_alphadec;
				break;
			case FBHLOP_TOGGLEVIS:
				f = fill_togglevis;
				break;
			default:
				warnx("unknown highlight op: %d", fbun.fbu_hlnc.fbhl_op);
				break;
			}

			if (f == NULL)
				break;

			switch (fbun.fbu_hlnc.fbhl_nc) {
			case NC_ALL:
				nc_runall(f);
				break;
			case NC_SELDM:
				nc_runsn(f);
				break;
			case NC_NONE:
				/* XXX ? */
				break;
			default:
				nc_apply(f, fbun.fbu_hlnc.fbhl_nc);
				break;
			}
			break;
		case FHT_CAPTION: {
			static char caption[BUFSIZ];

			if (fread(&fbun, 1, sizeof(struct fbcaption),
			    flyby_fp) != sizeof(struct fbcaption))
				err(1, "flyby read caption");
			snprintf(caption, sizeof(caption), "%s",
			    fbun.fbu_cap.fbcap_caption);
			caption_set(caption);
			break;
		    }
		default:
			if (fseek(flyby_fp, fbh.fbh_len, SEEK_CUR) == -1)
				err(1, "flyby fseek");
			break;
		}
	} while (!done);

	optdiff = oldopts ^ st.st_opts;
	for (i = 0; i < NOPS; i++)
		if (optdiff & (1 << i) &&
		    options[i].opt_flags & OPF_FBIGN)
			optdiff &= ~(1 << i);
	st.st_opts = oldopts;
	st.st_rf |= oldrf;
	st.st_rf &= ~RF_DATASRC;
	opt_flip(optdiff);

	if (st.st_opts & OP_REEL && reelframe_list.ol_cur > 0 &&
	    flyby_pos++ % (flyby_len / reelframe_list.ol_cur) == 0)
		reel_advance();
}

void
flyby_end(void)
{
	if (flyby_fp != NULL) {
		fclose(flyby_fp);
		flyby_fp = NULL;
	}

	switch (flyby_mode) {
	case FBM_PLAY:
		glutKeyboardFunc(gl_keyh_default);
		glutSpecialFunc(gl_spkeyh_default);
		glutMotionFunc(gl_motionh_default);
		glutPassiveMotionFunc(gl_pasvmotionh_default);
		glutMouseFunc(gl_mouseh_default);
		cursor_set(GLUT_CURSOR_LEFT_ARROW); /* XXX INHERIT */

		caption_set(NULL);
		load_state(&sav_st);

		if (st.st_opts & OP_REEL)
			reel_end();
		break;
	}
	flyby_mode = FBM_OFF;
	flyby_rstautoto();
}

__inline void
flyby_update(void)
{
	switch (flyby_mode) {
	case FBM_PLAY:		/* Replay. */
		/* XXX: move to draw()? */
		if ((st.st_opts & OP_STOP) == 0)
			flyby_read();
		break;
	case FBM_REC:		/* Record user commands. */
		flyby_writeseq(&st);
		break;
	}
}

/*
 * Reset auto-flyby timeout.
 *
 * flyby_nautoto keeps track of the number of resets that have been
 * triggered.  Once it hits zero, no more reset actions have been
 * triggered, and the auto-flyby may commence (see cocb_autoto).
 *
 * For example, when the user moves the mouse, a flurry of resets are
 * triggered, and when the timer for each expires, nautoto gets
 * decremented, until the final one expires.  When that happens, and the
 * callback is called, which decrements nautoto to zero, enabling the
 * auto flyby.
 */
void
flyby_rstautoto(void)
{
	if (st.st_opts & OP_AUTOFLYBY &&
	    flyby_mode == FBM_OFF) {
		flyby_nautoto++;
		glutTimerFunc(flyby_autoto, cocb_autoto, 0);
	}
}

void
flyby_beginauto(void)
{
	int ops;

	ops = st.st_opts;

	st.st_opts |= OP_LOOPFLYBY;
	flyby_begin(FBM_PLAY);

	sav_st.st_opts = ops;
}

void
flyby_clear(void)
{
	unlink(flyby_fn); /* XXX remove()? */
	errno = 0;
}

char *
flyby_set(const char *fn, int flags)
{
	int pids;

	pids = PANEL_FBCHO;
	if ((flags & CHF_DIR) == 0) {
		snprintf(flyby_fn, sizeof(flyby_fn),
		    "%s/%s", flyby_dir, fn);
		pids |= PANEL_FLYBY;
	}
	panel_rebuild(pids);
	return (flyby_dir);
}
