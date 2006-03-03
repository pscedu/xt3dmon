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

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	if (eggs & EGG_BORG) {
		/* Save current state. */
		ost = st;

		st.st_vmode = VM_WIREDONE;
		st.st_mode = SM_BORG;
		opt_enable(OP_TEX);
		opt_disable(OP_GROUND);

		vec_set(&st.st_v, -89.0, 25.0, 35.0);
		vec_set(&st.st_lv, 1.0, 0.0, 0.0);
		vec_set(&st.st_uv, 0.0, 1.0, 0.0);
	} else {
		opt_flip(st.st_opts ^ ost.st_opts);
		st.st_vmode = ost.st_vmode;
		st.st_mode = ost.st_mode;
//		st = ost;

		st.st_v = ost.st_v;
		st.st_lv = ost.st_lv;
		st.st_uv = ost.st_uv;
	}
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_SMODE | RF_GROUND;	/* XXX: RF_INIT ? */
}

void
egg_matrix(void)
{
	static struct fill ofill_bg;
	static struct state ost;

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	if (eggs & EGG_MATRIX) {
		ost = st;

		st.st_vmode = VM_PHYSICAL;
		st.st_mode = SM_MATRIX;
		opt_enable(OP_NLABELS);
		opt_disable(OP_GROUND | OP_WIREFRAME);

		vec_set(&st.st_v, 128.0, 6.25, 8.0);
		vec_set(&st.st_lv, -1.0, 0.0, 0.0);
		vec_set(&st.st_uv, 0.0, 1.0, 0.0);

		ofill_bg = fill_bg;
		fill_bg.f_r = 0.0;
		fill_bg.f_g = 0.2;
		fill_bg.f_b = 0.0;
	} else {
		opt_flip(st.st_opts ^ ost.st_opts);
		/* Restore original mode unless it was changed. */
		st.st_mode = st.st_mode == SM_MATRIX ?
		    ost.st_mode : st.st_mode;

		st.st_v = ost.st_v;
		st.st_lv = ost.st_lv;
		st.st_uv = ost.st_uv;

		fill_bg = ofill_bg;
	}
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_SMODE | RF_GROUND;
}
