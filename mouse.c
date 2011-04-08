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

#include <limits.h>
#include <stdlib.h>

#include "cdefs.h"
#include "cam.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "node.h"
#include "panel.h"
#include "select.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"
#include "xmath.h"

struct ivec	 mousev;
int		 spkey;
int		 motion_select_id;
int		 motion_select_reset;
struct panel	*panel_mobile;


void
gl_mouseh_null(__unusedx int button, __unused int state, __unused int u, __unused int v)
{
//	spkey = glutGetModifiers();
//	flyby_rstautoto();
}

void
gl_mouseh_default(__unusedx int button, __unused int state, int u, int v)
{
	static int reset;
	struct glname *gn;

	spkey = glutGetModifiers();
	if (button == GLUT_LEFT_BUTTON) {
		switch (state) {
		case GLUT_DOWN:
			gn = gl_select(0);
			reset = (gn != NULL);

			/*
			 * If the user selected an object, do not
			 * perform the "default" motion handler
			 * (i.e. focus revolve) until declick.
			 */
			if (spkey & GLUT_ACTIVE_SHIFT &&
			    gn != NULL) {
				glutMotionFunc(gl_motionh_select);
				motion_select_reset = 1;
				motion_select_id = gn->gn_arg_int;
			}
			break;
		case GLUT_UP:
			if (panel_mobile != NULL) {
				panel_demobilize(panel_mobile);
				panel_mobile = NULL;
				glutMotionFunc(gl_motionh_default);
			} else if (reset)
				glutMotionFunc(gl_motionh_default);
			break;
		}
	}

	mousev.iv_x = u;
	mousev.iv_y = v;

	flyby_rstautoto();
}

void
gl_motionh_panel(int u, int v)
{
	int du = u - mousev.iv_x, dv = v - mousev.iv_y;

	if (abs(du) + abs(dv) <= 1)
		return;
	mousev.iv_x = u;
	mousev.iv_y = v;

	panel_mobile->p_u += du;
	panel_mobile->p_v -= dv;
	panel_mobile->p_opts |= POPT_DIRTY;

	flyby_rstautoto();
}

void
gl_motionh_select(int u, int v)
{
	struct glname *gn;
	double du, dv;

	du = u - mousev.iv_x;
	dv = v - mousev.iv_y;

	if (abs(du) + abs(dv) <= 1)
		return;
	mousev.iv_x = u;
	mousev.iv_y = v;

	gn = gl_select(SPF_PROBE);
	if (motion_select_reset ||
	    (gn && motion_select_id != gn->gn_arg_int)) {
		gl_select(0);
		motion_select_reset = 0;
		motion_select_id = gn->gn_arg_int;
	}

	flyby_rstautoto();
}

void
gl_motionh_default(int u, int v)
{
	double du, dv;

	du = u - mousev.iv_x;
	dv = v - mousev.iv_y;

	if (abs(du) + abs(dv) <= 1)
		return;

	tween_push();
	if (spkey & GLUT_ACTIVE_SHIFT) {
		struct fvec center;

		center.fv_x = st.st_v.fv_x + st.st_lv.fv_x;
		center.fv_y = st.st_v.fv_y + st.st_lv.fv_y;
		center.fv_z = st.st_v.fv_z + st.st_lv.fv_z;
		cam_revolve(&center, 1, du * 0.01, -dv * 0.01, REVT_LKCEN);
	} else
		cam_revolvefocus(du * 0.01, -dv * 0.01);
	tween_pop();

	mousev.iv_x = u;
	mousev.iv_y = v;

	flyby_rstautoto();
}

void
gl_pasvmotionh_default(int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;

	gl_select(SPF_PROBE);

	flyby_rstautoto();
}

void
gl_pasvmotionh_null(int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;

	flyby_rstautoto();
}

void
gl_motionh_null(int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;

	flyby_rstautoto();
}

void
gl_mwheel_default(__unusedx int wheel, int dir, int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;

	gl_select(dir > 0 ? SPF_SQUIRE : SPF_DESQUIRE);

	flyby_rstautoto();
}
