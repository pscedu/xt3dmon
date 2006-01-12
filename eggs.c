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

	opts = st.st_opts;
	tween_push(TWF_LOOK | TWF_POS);

	if (eggs & EGG_BORG) {
		/* Save current state. */
		ost = st;

		st.st_vmode = VM_WIREDONE;
		st.st_mode = SM_BORG;
		st.st_opts |= OP_TEX;
		st.st_opts &= ~OP_GROUND;

		st.st_x = -89.0;  st.st_lx = 1.0;
		st.st_y =  25.0;  st.st_ly = 0.0;
		st.st_z =  35.0;  st.st_lz = 0.0;
	} else {
		/* Restore state from before borg mode. */
		st.st_opts = ost.st_opts;
		st.st_vmode = ost.st_vmode;

		st.st_x = ost.st_x;  st.st_lx = ost.st_lx;
		st.st_y = ost.st_y;  st.st_ly = ost.st_ly;
		st.st_z = ost.st_z;  st.st_lz = ost.st_lz;
	}
	tween_pop(TWF_LOOK | TWF_POS);

	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_SMODE;	/* XXX: RF_INIT ? */
	refresh_state(opts);
}

void
egg_matrix(void)
{
	static struct state ost;
	int opts;

	opts = st.st_opts;

	if (eggs & EGG_MATRIX) {
		ost = st;

		st.st_mode = SM_MATRIX;
		st.st_opts |= OP_NLABELS;
		st.st_opts &= ~OP_GROUND;
	} else {
		st.st_opts = ost.st_opts;
	}
	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_SMODE;
	refresh_state(opts);
}
