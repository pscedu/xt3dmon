/* $Id$ */

#include "compat.h"
#include "mon.h"

int eggs;

/*
 * Borg mode Easter egg
 */
void
egg_borg(void)
{
	static struct state ost;
	int opts;

	if (eggs & EGG_BORG) {
		/* Save current state */
		ost = st;

		/* Set up Borg view =) */
		st.st_vmode = VM_WIREDONE;
		st.st_opts |= OP_TEX;
		st.st_opts &= (~OP_GROUND);

		tween_push(TWF_LOOK | TWF_POS);
		st.st_x = -89.0;  st.st_lx = 1.0;
		st.st_y =  25.0;  st.st_ly = 0.0;
		st.st_z =  35.0;  st.st_lz = 0.0;
		tween_pop(TWF_LOOK | TWF_POS);

		refresh_state(ost.st_opts);
	} else {
		/* Restore state from before borg mode */
		opts = st.st_opts;
		st.st_opts = ost.st_opts;
		st.st_vmode = ost.st_vmode;

		tween_push(TWF_LOOK | TWF_POS);
		st.st_x = ost.st_x;  st.st_lx = ost.st_lx;
		st.st_y = ost.st_y;  st.st_ly = ost.st_ly;
		st.st_z = ost.st_z;  st.st_lz = ost.st_lz;
		tween_pop(TWF_LOOK | TWF_POS);

		/* refresh_state(opts); */
	}
	/* XXX: RF_INIT ? */
	st.st_rf |= RF_TEX | RF_CLUSTER | RF_SELNODE;
}

void
egg_matrix(void)
{
	static struct state ost;
	int opts;

	if (eggs & EGG_MATRIX) {
		st.st_opts |= OP_NLABELS;

		refresh_state(ost.st_opts);
	} else {
		opts = st.st_opts;
		st.st_opts = ost.st_opts;

		refresh_state(opts);
	}
}
