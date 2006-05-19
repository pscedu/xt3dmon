/* $Id$ */

#include "mon.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>

#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "pathnames.h"
#include "queue.h"
#include "reel.h"
#include "selnode.h"
#include "state.h"
#include "xmath.h"

#define FHT_INIT	1
#define FHT_SEQ		2
#define FHT_SELNODE	3
#define FHT_PANEL	4

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
};

union fbun {
	struct fbinit		fbu_init;
	struct fbpanel		fbu_panel;
	struct fbselnode	fbu_sn;
};

void		 init_panels(int);

int		 flyby_mode = FBM_OFF;
int		 flyby_autoto = 2 * 60 * 60; /* 20 seconds? */
int		 flyby_nautoto;
int		 flyby_len;
int		 flyby_pos;
static FILE	*flyby_fp;
struct state	 sav_st;

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

/* Open the flyby data file appropriately. */
void
flyby_begin(int mode)
{
	if (flyby_fp != NULL)
		return;

	switch (mode) {
	case FBM_PLAY:
		if ((flyby_fp = fopen(_PATH_FLYBY, "rb")) == NULL) {
			if (errno == ENOENT)
				return;
			err(1, "%s", _PATH_FLYBY);
		}
		flyby_mode = FBM_PLAY;

		/*
		 * Backup current options and enable/disable
		 * special options for the flyby duration.
		 */
		sav_st = st;
		st.st_opts &= ~OP_TWEEN;

//		panel_show(PANEL_FLYBY);
		if (st.st_opts & OP_REEL) {
			panel_show(PANEL_REEL);
			flyby_calclen();
			flyby_pos = 0;
			reel_start();
		}

		glutMotionFunc(gl_motionh_null);
		glutPassiveMotionFunc(gl_pasvmotionh_null);
		glutKeyboardFunc(gl_keyh_actflyby);
		glutSpecialFunc(gl_spkeyh_actflyby);
		glutMouseFunc(gl_mouseh_null);
		break;
	case FBM_REC:
		if ((flyby_fp = fopen(_PATH_FLYBY, "ab")) == NULL)
			err(1, "%s", _PATH_FLYBY);
		flyby_mode = FBM_REC;
		flyby_writeinit(&st);
		break;
	}
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
flyby_writeseq(struct state *st)
{
	flyby_writemsg(FHT_SEQ, st, sizeof(*st));
}

void
flyby_writeinit(struct state *st)
{
	struct fbinit fbi;
	struct panel *p;

	/* One copy per init msg should be okay. */
	fbi.fbi_state = *st;
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
flyby_writeselnode(int nid)
{
	flyby_writemsg(FHT_SELNODE, &nid, sizeof(nid));
}

/* Read a set of flyby data. */
void
flyby_read(void)
{
	int i, done, oldrf, oldopts, optdiff;
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
					reel_pos = 0;
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
			    RF_SELNODE | RF_HLNC;
			init_panels(fbun.fbu_init.fbi_panels);
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
				sn_toggle(n);
			break;
		default:
			if (fseek(flyby_fp, fbh.fbh_len, SEEK_CUR) == -1)
				err(1, "flyby fseek");
			break;
		}
	} while (!done);

	optdiff = oldopts ^ st.st_opts;
	for (i = 0; i < NOPS; i++)
		if (optdiff & (1 << i) &&
		    opts[i].opt_flags & OPF_FBIGN)
			optdiff &= ~(1 << i);
	st.st_opts = oldopts;
	st.st_rf |= oldrf;
	st.st_rf &= ~(RF_DATASRC | RF_REEL);
	opt_flip(optdiff);

	if (st.st_opts & OP_REEL &&
	    flyby_pos++ % (flyby_len / reelent_list.ol_cur) == 0) {
		reel_advance();
		st.st_rf |= RF_DATASRC;
	}
}

void
flyby_end(void)
{
	int rf, opts;

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

		rf = RF_CAM;
		if (st.st_opts & OP_REEL) {
			reel_end();
			rf |= RF_DATASRC;
		}

		if (st.st_dmode != sav_st.st_dmode)
			rf |= RF_DMODE;
		if (st.st_vmode != sav_st.st_vmode ||
		    st.st_pipemode != sav_st.st_pipemode ||
		    !ivec_eq(&st.st_wioff, &sav_st.st_wioff) ||
		    !ivec_eq(&st.st_winsp, &sav_st.st_winsp))
			rf |= RF_CLUSTER;
		if (st.st_hlnc != sav_st.st_hlnc)
			rf |= RF_HLNC;
		egg_toggle(st.st_eggs ^ sav_st.st_eggs);

		opts = st.st_opts;
		st = sav_st;
		st.st_opts = opts;
		st.st_rf = rf;

		opt_flip(st.st_opts ^ sav_st.st_opts);
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

/* Reset auto-flyby timeout. */
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
	unlink(_PATH_FLYBY); /* XXX remove()? */
	errno = 0;
}
