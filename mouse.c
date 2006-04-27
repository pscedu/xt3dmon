/* $Id$ */

#include "mon.h"

#include <limits.h>
#include <stdlib.h>

#include "cdefs.h"
#include "cam.h"
#include "env.h"
#include "gl.h"
#include "node.h"
#include "panel.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"
#include "xmath.h"

struct ivec	 mousev;
int	 	 spkey;
struct panel	*panel_mobile;

void
gl_mouseh_null(__unused int button, __unused int state, __unused int u, __unused int v)
{
//	spkey = glutGetModifiers();
}

void
gl_mouseh_default(__unused int button, __unused int state, int u, int v)
{
	spkey = glutGetModifiers();
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		glutDisplayFunc(gl_displayh_select);

	if (state == GLUT_UP && panel_mobile != NULL) {
		panel_demobilize(panel_mobile);
		panel_mobile = NULL;
		glutMotionFunc(gl_motionh_default);
	}

	mousev.iv_x = u;
	mousev.iv_y = v;
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
}

void
revolve_center_cluster(struct fvec *cen)
{
	double dist;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		cen->fv_x = XCENTER;
		cen->fv_y = YCENTER;
		cen->fv_z = ZCENTER;
		break;
	case VM_WIRED:
		dist = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH) / 4.0;
		cen->fv_x = st.st_x + st.st_lx * dist;
		cen->fv_y = st.st_y + st.st_ly * dist;
		cen->fv_z = st.st_z + st.st_lz * dist;
		break;
	case VM_WIREDONE:
		cen->fv_x = st.st_winsp.iv_x * (WIDIM_WIDTH  / 2.0f + st.st_wioff.iv_x);
		cen->fv_y = st.st_winsp.iv_y * (WIDIM_HEIGHT / 2.0f + st.st_wioff.iv_y);
		cen->fv_z = st.st_winsp.iv_z * (WIDIM_DEPTH  / 2.0f + st.st_wioff.iv_z);
		break;
	}
}

void
revolve_center_selnode(struct fvec *cen)
{
	struct selnode *sn;
	struct node *n;

	if (nselnodes == 0) {
		revolve_center_cluster(cen);
		return;
	}
	vec_set(cen, 0.0, 0.0, 0.0);
	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;

		switch (st.st_vmode) {
		case VM_WIRED:
			break;
		}

		cen->fv_x += n->n_v->fv_x + n->n_dimp->fv_w / 2;
		cen->fv_y += n->n_v->fv_y + n->n_dimp->fv_h / 2;
		cen->fv_z += n->n_v->fv_z + n->n_dimp->fv_d / 2;
	}
	cen->fv_x /= nselnodes;
	cen->fv_y /= nselnodes;
	cen->fv_z /= nselnodes;
}

void (*revolve_centerf)(struct fvec *) = revolve_center_selnode;

void
gl_motionh_default(int u, int v)
{
	int du = u - mousev.iv_x, dv = v - mousev.iv_y;
	struct fvec center;

	if (abs(du) + abs(dv) <= 1)
		return;

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	if (spkey & GLUT_ACTIVE_SHIFT) {
		center.fv_x = st.st_v.fv_x + st.st_lv.fv_x;
		center.fv_y = st.st_v.fv_y + st.st_lv.fv_y;
		center.fv_z = st.st_v.fv_z + st.st_lv.fv_z;
	} else
		(*revolve_centerf)(&center);
	cam_revolve(&center, (double)du, (double)-dv);
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	mousev.iv_x = u;
	mousev.iv_y = v;
}

void
gl_pasvmotionh_default(int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;

	glutDisplayFunc(gl_displayh_selectprobe);
}

void
gl_pasvmotionh_null(int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;
}

void
gl_motionh_null(int u, int v)
{
	mousev.iv_x = u;
	mousev.iv_y = v;
}
