/* $Id$ */

#include <err.h>
#include <errno.h>
#include <stdio.h>

#include "mon.h"

static FILE *flyby_fp;
struct flyby fb;

void init_panels(void);

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
//		rebuild(RO_COMPILE); /* XXX: is this needed? */
		init_panels();
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
	/* Save the node ID. */
	if (selnode != NULL)
		fb.fb_nid = selnode->n_nid;
	else
		fb.fb_nid = -1;

	if (fwrite(&st, sizeof(struct state), 1, flyby_fp) != 1)
		err(1, "fwrite st");
	if (fwrite(&fb, sizeof(struct flyby), 1, flyby_fp) != 1)
		err(1, "fwrite fb");
}

/* Read a set of flyby data. */
void
read_flyby(void)
{
	struct node *n;
	int tnid, oldopts;

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
			adjcam();
			return;
		}
	}

	if ((st.st_ro & OP_TWEEN) == 0)
		adjcam();

	/* Restore selected node. */
	if (fb.fb_nid != -1) {
		/* Force recompile if needed. */
		if (tnid != fb.fb_nid)
			st.st_ro |= RO_SELNODE;
		n = node_for_nid(fb.fb_nid);
		if (n != NULL)
			select_node(n);
	} else
		selnode = NULL;
	if (fb.fb_panels)
		flip_panels(fb.fb_panels);
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
}

void
update_flyby(void)
{
	int clear = build_flyby || active_flyby;

	/* Record user commands. */
	if (build_flyby)
		write_flyby();
	/* Replay. */
	else if (active_flyby)
		read_flyby();
	if (clear) {
		st.st_ro = 0;
		fb.fb_panels = 0;
	}
}

/*
 * Detect which panels flipped between flyby states.
 */
void
flip_panels(int panels)
{
	int b;

	panels = fb.fb_panels;
	while ((b = ffs(panels))) {
		panel_toggle(1 << (b - 1));
		panels &= ~(1 << (b - 1));
	}
}

/*
 * Set panel state from the current state to the first state in the
 * flyby.
 */
void
init_panels(void)
{
	struct panel *p;
	int cur;

	cur = 0;
	TAILQ_FOREACH(p, &panels, p_link)
		cur |= p->p_id;
	flip_panels(cur ^ fb.fb_panels);
}
