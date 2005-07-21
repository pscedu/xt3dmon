/* $Id$ */

#include <err.h>
#include <errno.h>
#include <stdio.h>

#include "mon.h"

static FILE *flyby_fp;
struct flyby fb;

void init_panels(int);

/* Open the flyby data file appropriately. */
void
begin_flyby(char m)
{
	if (flyby_fp != NULL)
		return;

	if (m == 'r') {
		if ((flyby_fp = fopen(_PATH_FLYBY, "rb")) == NULL) {
			if (errno == ENOENT)
				return;
			err(1, "%s", _PATH_FLYBY);
		}
		active_flyby = 1;
		rebuild(RF_INIT);
	} else if (m == 'w') {
		if ((flyby_fp = fopen(_PATH_FLYBY, "ab")) == NULL)
			err(1, "%s", _PATH_FLYBY);
		build_flyby = 1;
	}
}

/* Write current data for flyby. */
void
write_flyby(void)
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

//	save = fb.fb_panels;
	fb.fb_panels &= ~FB_PMASK;		/* XXX:  stupid. */
	if (fwrite(&fb, sizeof(struct flyby), 1, flyby_fp) != 1)
		err(1, "fwrite fb");
//	fb.fb_panels = save;
}

/* Read a set of flyby data. */
void
read_flyby(void)
{
	static int stereo_left;
	int tnid, oldopts;
	struct node *n;

	if (st.st_opts & OP_STEREO) {
		stereo_left = !stereo_left;
		if (!stereo_left) {
			cam_move(CAMDIR_RIGHT);
			cam_move(CAMDIR_RIGHT);
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
			active_flyby = 0;
			end_flyby();
			glutKeyboardFunc(keyh_default);
			glutSpecialFunc(spkeyh_default);
			glutMotionFunc(m_activeh_default);
			glutPassiveMotionFunc(m_passiveh_default);
			cam_update();
			return;
		}
	}

	if (st.st_opts & OP_STEREO && stereo_left) {
		cam_move(CAMDIR_LEFT);
		cam_revolve(-1);
	}

	/* XXX:  is this right? */
	if ((st.st_rf & OP_TWEEN) == 0)
		cam_update();

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
end_flyby(void)
{
	if (flyby_fp != NULL) {
		fclose(flyby_fp);
		flyby_fp = NULL;
	}
	active_flyby = build_flyby = 0;
	fb.fb_panels = 0;
}

void
update_flyby(void)
{
	/* Record user commands. */
	if (build_flyby)
		write_flyby();
	/* Replay. */
	else if (active_flyby && !(st.st_opts & OP_STOP))
		read_flyby();
	fb.fb_panels = 0;
}

void
flip_panels(int panels)
{
	int b;

	while (panels) {
		b = ffs(panels) - 1;
		panels &= ~(1 << b);
		panel_toggle(1 << b);
	}
}

/*
 * Set panel state from the current state to the first state in the
 * flyby.
 */
void
init_panels(int start)
{
	struct panel *p;
	int cur;

	cur = 0;
	TAILQ_FOREACH(p, &panels, p_link)
		cur |= p->p_id;
	flip_panels((cur ^ start) & ~FB_PMASK);
}
