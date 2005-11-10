/* $Id$ */

#include "compat.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>

#include "mon.h"

int		 flyby_mode;
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

struct fbhljst {
	int		fbhl_state;
};

union fbun {
	struct fbinit		fbu_init;
	struct fbpanel		fbu_panel;
	struct fbselnode	fbu_sn;
	struct fbhljst		fbu_hljst;
};

#define FHT_INIT	1
#define FHT_SEQ		2
#define FHT_SELNODE	3
#define FHT_PANEL	4
#define FHT_HLJSTATE	5

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

		glutMotionFunc(m_activeh_null);
		glutPassiveMotionFunc(m_passiveh_null);
		glutKeyboardFunc(keyh_actflyby);
		glutSpecialFunc(spkeyh_actflyby);
		glutMouseFunc(mouseh_null);
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
	int save;

	save = st->st_opts;
	st->st_opts &= ~FB_OMASK;
	flyby_writemsg(FHT_SEQ, st, sizeof(*st));
	st->st_opts = save;
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

	fbi.fbi_state.st_opts &= ~FB_OMASK;
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

__inline void
flyby_writehljstate(int state)
{
	flyby_writemsg(FHT_HLJSTATE, &state, sizeof(state));
}

/* Read a set of flyby data. */
void
flyby_read(void)
{
	int done, oldopts;
	struct fbhdr fbh;

	oldopts = st.st_opts;

	done = 0;
	do {
		if (fread(&fbh, 1, sizeof(fbh), flyby_fp) != sizeof(fbh)) {
			if (feof(flyby_fp)) {
				if (st.st_opts & OP_LOOPFLYBY)
					rewind(flyby_fp);
				else {
					flyby_end();
					return;
				}
			} else
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
			if (fbp.fbp_panel > 0 && fbp.fbp_panel < NPANELS)
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
		case FHT_HLJSTATE: {
			struct fbhljst fbhl;
			if (fread(&fbhl, 1, sizeof(fbhl), flyby_fp) !=
			    sizeof(fbhl))
				err(1, "flyby read panel");
			hl_jstate = fbhl.fbhl_state;
			hl_refresh();
			break;
		    }
		}
	} while (!done);

#if 0
	/*
	 * A new message type should be created that
	 * gets written to the file whenever (v,lv,uv)
	 * changes.
	 */
	if (ost.st_v != ost.st_v ||
	    ost.st_lv != ost.st_lv ||
	    ost.st_uv != ost.st_uv)
		cam_dirty = 1;
#endif

	/* XXX:  is this right? */
	if ((st.st_rf & OP_TWEEN) == 0)
		cam_dirty = 1;

	st.st_opts ^= ((st.st_opts ^ oldopts) & FB_OMASK);
	refresh_state(oldopts);
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
		glutKeyboardFunc(keyh_default);
		glutSpecialFunc(spkeyh_default);
		glutMotionFunc(m_activeh_default);
		glutPassiveMotionFunc(m_passiveh_default);
		glutMouseFunc(mouseh_default);
		cam_dirty = 1;
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
