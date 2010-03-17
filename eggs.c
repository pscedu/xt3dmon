/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#include "mon.h"

#include "fill.h"
#include "state.h"
#include "tween.h"

void
egg_borg(void)
{
	static struct state ost;

	tween_push();
	if (st.st_eggs & EGG_BORG) {
		/* Save current state. */
		ost = st;

		st.st_vmode = VM_WIONE;
		st.st_dmode = DM_BORG;
		opt_enable(OP_TEX);
		opt_disable(OP_GROUND);

		vec_set(&st.st_v, -89.0, 25.0, 35.0);
		vec_set(&st.st_lv, 1.0, 0.0, 0.0);
		st.st_ur = 0.0;
	} else {
		opt_flip(st.st_opts ^ ost.st_opts);
		st.st_vmode = ost.st_vmode;
		st.st_dmode = ost.st_dmode;
//		st = ost;

		st.st_v = ost.st_v;
		st.st_lv = ost.st_lv;
		st.st_ur = ost.st_ur;
	}
	tween_pop();

	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_DMODE | RF_GROUND | RF_VMODE;
}

void
egg_matrix(void)
{
	static struct state ost;

	tween_push();
	if (st.st_eggs & EGG_MATRIX) {
		ost = st;

		st.st_vmode = VM_PHYS;
		st.st_dmode = DM_MATRIX;
		opt_enable(OP_NLABELS);
		opt_disable(OP_GROUND | OP_FRAMES);

		vec_set(&st.st_v, 128.0, 6.25, 8.0);
		vec_set(&st.st_lv, -1.0, 0.0, 0.0);
		st.st_ur = 0.0;
		st.st_urev = 0;

		glClearColor(0.0, 0.2, 0.0, 1.0);
	} else {
		opt_flip(st.st_opts ^ ost.st_opts);
		/* Restore original mode unless it was changed. */
		st.st_dmode = st.st_dmode == DM_MATRIX ?
		    ost.st_dmode : st.st_dmode;

		st.st_v = ost.st_v;
		st.st_lv = ost.st_lv;
		st.st_ur = ost.st_ur;
		st.st_urev = 0;

		glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
	}
	tween_pop();

	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_DMODE | RF_GROUND | RF_VMODE;
}

void
egg_hackers(void)
{
	static struct state ost;

	tween_push();
	if (st.st_eggs & EGG_HACKERS) {
		ost = st;

		st.st_vmode = VM_PHYS;
		st.st_dmode = DM_HACKERS;
		opt_set(OP_GROUND | OP_FRAMES);

		vec_set(&st.st_v, 128.0, 6.25, 8.0);
		vec_set(&st.st_lv, -1.0, 0.0, 0.0);
		st.st_ur = 0.0;
		st.st_urev = 0;

		glClearColor(0.0, 0.0, 0.0, 1.0);
	} else {
		opt_set(ost.st_opts);
		st.st_dmode = ost.st_dmode;
		st.st_v = ost.st_v;
		st.st_lv = ost.st_lv;
		st.st_ur = ost.st_ur;
		st.st_urev = 0;

		glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
	}
	tween_pop();

	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_DMODE | RF_GROUND | RF_VMODE;
}

void
egg_toggle(int diff)
{
	st.st_eggs ^= diff;
	if (diff & EGG_BORG)
		egg_borg();
	if (diff & EGG_MATRIX)
		egg_matrix();
	if (diff & EGG_HACKERS)
		egg_hackers();
}

