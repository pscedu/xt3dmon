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
#include "selnode.h"
#include "state.h"

int		 flyby_mode;
int		 sav_opts;
static FILE	*flyby_fp;

void		 init_panels(int);

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

#define FHT_INIT	1
#define FHT_SEQ		2
#define FHT_SELNODE	3
#define FHT_PANEL	4

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

		sav_opts = st.st_opts;
		st.st_opts &= ~OP_TWEEN;

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
	int i, done, oldopts, optdiff;
	struct fbhdr fbh;

	oldopts = st.st_opts;

	done = 0;
	do {
		if (fread(&fbh, 1, sizeof(fbh), flyby_fp) != sizeof(fbh)) {
			if (feof(flyby_fp)) {
				if (st.st_opts & OP_LOOPFLYBY) {
					rewind(flyby_fp);
					continue;
				} else {
					flyby_end();
					return;
				}
			}
			err(1, "fread flyby header");
		}
		switch (fbh.fbh_type) {
		case FHT_INIT: {
			struct fbinit fbi;

			if (fread(&fbi, 1, sizeof(fbi), flyby_fp) !=
			    sizeof(fbi))
				err(1, "flyby read init");
			st = fbi.fbi_state;
			st.st_rf |= RF_INIT;
			init_panels(fbi.fbi_panels);
			done = 1;
			break;
		    }
		case FHT_SEQ:
			if (fread(&st, 1, sizeof(st), flyby_fp) !=
			    sizeof(st))
				err(1, "flyby read seq");
			done = 1;
			break;
		case FHT_PANEL: {
			struct fbpanel fbp;

			if (fread(&fbp, 1, sizeof(fbp), flyby_fp) !=
			    sizeof(fbp))
				err(1, "flyby read panel");
			if (fbp.fbp_panel >= 0 && fbp.fbp_panel < NPANELS)
				panel_toggle(fbp.fbp_panel);
			break;
		    }
		case FHT_SELNODE: {
			struct fbselnode fbsn;
			struct node *n;

			if (fread(&fbsn, 1, sizeof(fbsn), flyby_fp) !=
			    sizeof(fbsn))
				err(1, "flyby read panel");
			if ((n = node_for_nid(fbsn.fbsn_nid)) != NULL)
				sn_toggle(n);
			break;
		    }
		}
	} while (!done);

	optdiff = oldopts ^ st.st_opts;
	for (i = 0; i < NOPS; i++)
		if (optdiff & (1 << i) &&
		    opts[i].opt_flags & OPF_FBIGN)
			optdiff &= ~(1 << i);
	opt_flip(optdiff);
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
		opt_flip(st.st_opts ^ sav_opts);
		st.st_rf |= RF_CAM;
		/* rebuild(RF_INIT); */
		break;
	}

	flyby_mode = FBM_OFF;
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
