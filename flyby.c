/* $Id$ */

#include <err.h>
#include <errno.h>
#include <stdio.h>

#include "mon.h"

int		 flyby_mode;
static FILE	*flyby_fp;
struct flyby	 fb;

void		 init_panels(int);

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
		rebuild(RF_INIT);

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
	}
}

/* Write current data for flyby. */
void
flyby_write(void)
{
	int save;

	/* Save the node ID. */
	if (selnode != NULL)
		fb.fb_nid = selnode->n_nid;
	else
		fb.fb_nid = -1;

	save = st.st_opts;
	st.st_opts &= ~FB_OMASK;
	if (fwrite(&st, sizeof(struct state), 1, flyby_fp) != 1)
		err(1, "fwrite st");
	st.st_opts = save;

	fb.fb_panels &= ~FB_PMASK;		/* XXX:  stupid. */
	if (fwrite(&fb, sizeof(struct flyby), 1, flyby_fp) != 1)
		err(1, "fwrite fb");
}

/* Read a set of flyby data. */
void
flyby_read(void)
{
	static int stereo_left;
	int tnid, oldopts;
	struct node *n;

	if (st.st_opts & OP_STEREO) {
		stereo_left = !stereo_left;
		if (!stereo_left) {
			cam_move(CAMDIR_RIGHT, 0.02);
			cam_move(CAMDIR_RIGHT, 0.02);
			cam_revolve(1);
			cam_revolve(1);
			return;
		}
	}

	oldopts = st.st_opts;

	/* Save selected node. */
	tnid = fb.fb_nid;
again:
	if (fread(&st, sizeof(struct state), 1, flyby_fp) != 1 ||
	    fread(&fb, sizeof(struct flyby), 1, flyby_fp) != 1) {
		if (ferror(flyby_fp))
			err(1, "fread");
		/* End of flyby. */
		if (feof(flyby_fp)) {
			if (st.st_opts & OP_LOOPFLYBY) {
				rewind(flyby_fp);
				goto again;
			}
			flyby_mode = FBM_OFF;
			flyby_end();
			return;
		}
	}

	if (st.st_opts & OP_STEREO && stereo_left) {
		cam_move(CAMDIR_LEFT, 0.01);
		cam_revolve(-1);
	}

	/* XXX:  is this right? */
	if ((st.st_rf & OP_TWEEN) == 0)
		cam_update();

	/* XXX: this code is a mess. */
	/* Restore selected node. */
	if (fb.fb_nid != -1) {
		/* Force recompile if needed. */
		if (tnid != fb.fb_nid)
			st.st_rf |= RF_SELNODE;
		n = node_for_nid(fb.fb_nid);
		if (n != NULL)
			select_node(n);
	} else
		selnode = NULL;
	/* This is safe from the panel mask. */
	if (fb.fb_panels)
		flip_panels(fb.fb_panels);
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
	fb.fb_panels = 0;

	switch (flyby_mode) {
	case FBM_PLAY:
		glutKeyboardFunc(keyh_default);
		glutSpecialFunc(spkeyh_default);
		glutMotionFunc(m_activeh_default);
		glutPassiveMotionFunc(m_passiveh_default);
		glutMouseFunc(mouseh_default);
		cam_update();
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
		flyby_write();
		break;
	}
	fb.fb_panels = 0;
}
