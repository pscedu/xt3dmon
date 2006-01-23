/* $Id$ */

#include "compat.h"

#include <limits.h>
#include <stdlib.h>

#include "cdefs.h"
#include "mon.h"
#include "xmath.h"

int	 	 spkey, lastu, lastv;
struct panel	*panel_mobile;
void		(*gl_displayhp_old)(void);

void
gl_mouseh_null(__unused int button, __unused int state, __unused int u, __unused int v)
{
//	spkey = glutGetModifiers();
}

static __inline void
selfv_calc(struct fvec *fvp, int u, int v)
{
	struct fvec sph;

	vec_cart2sphere(&st.st_lv, &sph);
	sph.fv_t += DEG_TO_RAD(FOVY) * ASPECT * (u - win_width / 2.0) / win_width;
	sph.fv_p -= DEG_TO_RAD(FOVY) * (win_height / 2.0 - v) / win_height;
	vec_sphere2cart(&sph, fvp);
	vec_normalize(fvp);
}

void
gl_mouseh_default(__unused int button, __unused int state, int u, int v)
{
	spkey = glutGetModifiers();
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN &&
	    gl_displayhp != gl_displayh_select) {
		gl_displayhp_old = gl_displayhp;
		gl_displayhp = gl_displayh_select;
		glutDisplayFunc(gl_displayhp);
	}

	if (state == GLUT_UP && panel_mobile != NULL) {
		panel_demobilize(panel_mobile);
		panel_mobile = NULL;
		glutMotionFunc(gl_motionh_default);
	}

	lastu = u;
	lastv = v;
}

void
gl_motionh_panel(int u, int v)
{
	int du = u - lastu, dv = v - lastv;

	if (abs(du) + abs(dv) <= 1)
		return;
	lastu = u;
	lastv = v;

	panel_mobile->p_u += du;
	panel_mobile->p_v -= dv;
	panel_mobile->p_opts |= POPT_DIRTY;
}

void
revolve_center_cluster(struct fvec *cen)
{
	float dist;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		cen->fv_x = XCENTER;
		cen->fv_y = YCENTER;
		cen->fv_z = ZCENTER;
		break;
	case VM_WIRED:
		dist = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH);
		if (st.st_vmode == VM_WIRED)
			dist /= 3.0f;
		cen->fv_x = st.st_x + st.st_lx * dist;
		cen->fv_y = st.st_y + st.st_ly * dist;
		cen->fv_z = st.st_z + st.st_lz * dist;
		break;
	case VM_WIREDONE:
		cen->fv_x = st.st_winsp.iv_x * (WIDIM_WIDTH  / 2.0f + wioff.iv_x);
		cen->fv_y = st.st_winsp.iv_y * (WIDIM_HEIGHT / 2.0f + wioff.iv_y);
		cen->fv_z = st.st_winsp.iv_z * (WIDIM_DEPTH  / 2.0f + wioff.iv_z);
		break;
	}
}

void
revolve_center_selnode(struct fvec *cen)
{
	struct selnode *sn;

	if (nselnodes == 0) {
		revolve_center_cluster(cen);
		return;
	}
	cen->fv_x = cen->fv_y = cen->fv_z = 0.0f;
	SLIST_FOREACH(sn, &selnodes, sn_next) {
		cen->fv_x += sn->sn_nodep->n_v->fv_x + NODEWIDTH  / 2;
		cen->fv_y += sn->sn_nodep->n_v->fv_y + NODEHEIGHT / 2;
		cen->fv_z += sn->sn_nodep->n_v->fv_z + NODEDEPTH  / 2;
	}
	cen->fv_x /= nselnodes;
	cen->fv_y /= nselnodes;
	cen->fv_z /= nselnodes;
}

void (*revolve_centerf)(struct fvec *) = revolve_center_selnode;

void
gl_motionh_default(int u, int v)
{
	int du = u - lastu, dv = v - lastv;
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

	lastu = u;
	lastv = v;
}

void
gl_pasvmotionh_default(int u, int v)
{
	lastu = u;
	lastv = v;

//	if (drawh != drawh_select) {
//		drawh_old = drawh;
//		drawh = drawh_select;
//		glutDisplayFunc(drawh);
//	}
}

void
gl_pasvmotionh_null(int u, int v)
{
	lastu = u;
	lastv = v;
}

void
gl_motionh_null(int u, int v)
{
	lastu = u;
	lastv = v;
}
