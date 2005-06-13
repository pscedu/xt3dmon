/* $Id$ */

#include "mon.h"
#include<stdio.h>
#include<err.h>

#define FLYBY_FILE "flyby.data"
static FILE *flyby_fp;
struct flyby fb;

/* Open the flyby data file appropriately */
void begin_flyby(char m)
{
	if(flyby_fp != NULL)
		return;

	if(m == 'r')
	{
		if((flyby_fp = fopen(FLYBY_FILE, "rb")) == NULL)
			err(1, "%s", FLYBY_FILE);
	} else if (m == 'w') {
		if((flyby_fp = fopen(FLYBY_FILE, "ab")) == NULL)
			err(1, "%s", FLYBY_FILE);
	}
}

/* Write current data for flyby */
void write_flyby(void)
{
	int tnid = -1, tnlid;

	/* Save the node id instead of ptr */
	if(selnode != NULL) {

		/* Save node and items we need */
		tnid = selnode->n_nid;
		tnlid = selnode->n_logid;

		/* Switch and set nid */
		fb.fb_nid = tnid;
		fb.fb_nlid = tnlid;

	} else {
		fb.fb_nid = -1;
//		fb.fb_nlid = -1;
	}

	if(!fwrite(&st, sizeof(struct state), 1, flyby_fp))
		err(1, "flyby data write err");
	
	/* Set node back */
	if(tnid != -1)
		selnode = invmap[tnlid][tnid];
	else
		selnode = NULL;

}

/* Read a set of flyby data */
void read_flyby(void)
{
	if(!fread(&st, sizeof(struct state), 1, flyby_fp)) {

		/* end of flyby */
		if(feof(flyby_fp) != 0) {
			active_flyby = 0;
			glutKeyboardFunc(keyh_default);
			glutSpecialFunc(spkeyh_default);
		}
		else
			err(1, "flyby data read err");
	}

	if ((st.st_ro & OP_TWEEN) == 0)
		adjcam();

	/* Restore selected node */
	if(fb.fb_nid != -1) {

		int tnid, tnlid;
		tnid = fb.fb_nid;
		tnlid = fb.fb_nlid;
		selnode = invmap[tnlid][tnid];

		/* Force recompile */
		st.st_ro |= RO_COMPILE;
printf(" Restore Selected Node: COMPILE\n");
	} else {
		selnode = NULL;
printf(" No node selected\n");
	}

	restore_state(1);
}

void end_flyby(void)
{
	if(flyby_fp != NULL) {
		fclose(flyby_fp);
		flyby_fp = NULL;
	}
}

void update_flyby(void)
{
	/* record user commands */
	if(build_flyby) {
		write_flyby();
		st.st_ro = 0;
	}

	/* replay */
	if(active_flyby)
		read_flyby();
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
		panel_toggle(b);
		panels &= ~b;
	}
}

/*
 * Set panel state from the current state to the first state in the
 * flyby.
 */
void
init_panels(int new)
{
	struct panel *p;
	int cur;

	cur = 0;
	TAILQ_FOREACH(p, &panels, p_link)
		cur |= p->p_id;
	flip_panels(cur ^ new);
}

/*
 * Restore original panel state at end of flyby.
 */
void
restore_panels(int old, int new)
{
}
