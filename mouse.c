/* $Id$ */

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
int	 	 spkey;
int		 motion_select_ret;
struct panel	*panel_mobile;


void
gl_mouseh_null(__unused int button, __unused int state, __unused int u, __unused int v)
{
//	spkey = glutGetModifiers();
//	flyby_rstautoto();
}

void
gl_mouseh_default(__unused int button, __unused int state, int u, int v)
{
	static int ret;

	spkey = glutGetModifiers();
	if (button == GLUT_LEFT_BUTTON) {
		switch (state) {
		case GLUT_DOWN:
			ret = gl_select(0);

			/*
			 * If the user selected an object, do not
			 * perform the "default" motion handler
			 * (i.e. focus revolve) until declick.
			 */
			if (spkey & GLUT_ACTIVE_SHIFT &&
			    ret != SP_MISS) {
				glutMotionFunc(gl_motionh_select);
				motion_select_ret = SP_NONE;
			}
			break;
		case GLUT_UP:
			if (panel_mobile != NULL) {
				panel_demobilize(panel_mobile);
				panel_mobile = NULL;
				glutMotionFunc(gl_motionh_default);
			} else if (ret != SP_MISS) {
				glutMotionFunc(gl_motionh_default);
			}
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
	double du, dv;
	int ret;

	du = u - mousev.iv_x;
	dv = v - mousev.iv_y;

	if (abs(du) + abs(dv) <= 1)
		return;
	mousev.iv_x = u;
	mousev.iv_y = v;

	ret = gl_select(SPF_PROBE);
	if (motion_select_ret != ret)
		gl_select(0);
	motion_select_ret = ret;

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

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	if (spkey & GLUT_ACTIVE_SHIFT) {
		struct fvec center;

		center.fv_x = st.st_v.fv_x + st.st_lv.fv_x;
		center.fv_y = st.st_v.fv_y + st.st_lv.fv_y;
		center.fv_z = st.st_v.fv_z + st.st_lv.fv_z;
		cam_revolve(&center, 1, du * 0.01, -dv * 0.01, REVT_LKCEN);
	} else
		cam_revolvefocus(du * 0.01, -dv * 0.01, REVT_LKAVG);
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

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
gl_mwheel_default(__unused int wheel, int dir, int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;

	gl_select(dir > 0 ? SPF_SQUIRE : SPF_DESQUIRE);

	flyby_rstautoto();
}
