/* $Id$ */

#include "mon.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "cam.h"
#include "deusex.h"
#include "env.h"
#include "lnseg.h"
#include "mark.h"
#include "node.h"
#include "objlist.h"
#include "panel.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"

double curlyq_adj;

#define DST(a, b)					\
	(sqrt(						\
	    SQUARE((a)->fv_x - (b)->fv_x) +		\
	    SQUARE((a)->fv_y - (b)->fv_y) +		\
	    SQUARE((a)->fv_z - (b)->fv_z)))

void
dxi_orbit(void)
{
	double rx, ry, rz;

	tween_push(TWF_POS);
	do {
		rx = (rand() % 2 ? 1 : -1) * fmod(rand() / 100.0, ROWWIDTH);
		ry = (rand() % 2 ? 1 : -1) * fmod(rand() / 100.0, ROWWIDTH);
		rz = (rand() % 2 ? 1 : -1) * fmod(rand() / 100.0, ROWWIDTH);

		st.st_v = focus;
		st.st_x += rx;
		st.st_y += ry;
		st.st_z += rz;
	} while (DST(&focus, &st.st_v) < ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1));
	tween_pop(TWF_POS);
}

int
dx_orbit(int dim, int sign)
{
	static int t;
	double du, dv;
	int ret;

	ret = 0;
	du = dv = 0.0;
	switch (dim) {
	case DIM_X:
		du = sign * (2 * M_PI / 360 / .01);
		break;
	case DIM_Y:
		dv = sign * -(2 * M_PI / 360 / .01);
		break;
	}

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	cam_revolvefocus(du, dv);
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += 1;
	if (t >= 360) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_orbit_u(void)
{
	return (dx_orbit(DIM_X, 1));
}

int
dx_orbit_urev(void)
{
	return (dx_orbit(DIM_X, -1));
}

int
dx_orbit_v(void)
{
	return (dx_orbit(DIM_Y, 1));
}

int
dx_orbit_vrev(void)
{
	return (dx_orbit(DIM_Y, -1));
}

int
dx_orbit3(int dim, int sign)
{
	static int n = 0;
	int ret;

	if (dx_orbit(dim, sign))
		n++;

	ret = 0;
	if (n >= 3) {
		n = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_orbit3_u(void)
{
	return (dx_orbit3(DIM_X, 1));
}

int
dx_orbit3_urev(void)
{
	return (dx_orbit3(DIM_X, -1));
}

int
dx_orbit3_v(void)
{
	return (dx_orbit3(DIM_Y, 1));
}

int
dx_orbit3_vrev(void)
{
	return (dx_orbit3(DIM_Y, -1));
}

void
dxi_curlyq(void)
{
	dxi_orbit();
	curlyq_adj = DST(&st.st_v, &focus) / (180 * 3 + 90);
}

int
dx_curlyq(void)
{
	tween_push(TWF_POS);
	cam_move(DIR_FORWARD, curlyq_adj);
	tween_pop(TWF_POS);
	return (dx_orbit3(DIM_X, 1));
}

int
dx_cubanoid(int dim)
{
	static double t;
	struct fvec sv, uv, lv, xv, axis;
	double roll, a, b, max;
	float mag;
	int ret;

	ret = 0;

	switch (dim) {
	case DIM_X:
		a = CABHEIGHT/4.0;
		b = ROWDEPTH/4.0;
		break;
	case DIM_Y:
		a = ROWWIDTH/4.0;
		b = CABHEIGHT/2.0;
		break;
	case DIM_Z:
		a = ROWWIDTH/4.0 - CABWIDTH;
		b = CABHEIGHT/4.0;
		break;
	}
	max = MAX(a, b);

	if (t < 2 * M_PI) {
		sv.fv_x = a * sin(t - M_PI/2) + max;
		sv.fv_y = b * cos(t - M_PI/2);

		uv.fv_x = sin(t - M_PI/2);
		uv.fv_y = cos(t - M_PI/2);
	} else {
		sv.fv_x = -a * sin(t - M_PI/2) - max;
		sv.fv_y = b * cos(t - M_PI/2);

		uv.fv_x = sin(t - M_PI/2);
		uv.fv_y = -cos(t - M_PI/2);
	}
	sv.fv_z = 0.0;
	uv.fv_z = 0.0;

	vec_set(&xv, 0.0, 0.0, 1.0);
	vec_normalize(&uv);
	vec_crossprod(&lv, &uv, &xv);

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);

	switch (dim) {
	case DIM_X:
		st.st_x = 0;
		st.st_y = sv.fv_x;
		st.st_z = sv.fv_y;

		st.st_ux = 0.0;
		st.st_uy = uv.fv_x;
		st.st_uz = uv.fv_y;

		st.st_lx = 0.0;
		st.st_ly = lv.fv_x;
		st.st_lz = lv.fv_y;
		break;
	case DIM_Y:
		st.st_x = sv.fv_x;
		st.st_y = 0;
		st.st_z = sv.fv_y;

		st.st_ux = uv.fv_x;
		st.st_uy = 0.0;
		st.st_uz = uv.fv_y;

		st.st_lx = lv.fv_x;
		st.st_ly = 0.0;
		st.st_lz = lv.fv_y;
		break;
	case DIM_Z:
		st.st_x = sv.fv_x;
		st.st_y = sv.fv_y;
		st.st_z = 0;

		st.st_ux = uv.fv_x;
		st.st_uy = uv.fv_y;
		st.st_uz = 0.0;

		st.st_lx = lv.fv_x;
		st.st_ly = lv.fv_y;
		st.st_lz = 0.0;
		break;
	}

#if 0
	vec_rotate(&st.st_v, &axis, atan2(
	    (ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1)) / 2.0, ROWWIDTH));
#endif

	mag = vec_mag(&st.st_v);
	vec_set(&axis, 1.0, 0.0, 0.0);
	vec_rotate(&st.st_v, &axis, M_PI / 4.0);
	st.st_x = st.st_x * mag;
	st.st_y = st.st_y * mag;
	st.st_z = st.st_z * mag;
	st.st_x += XCENTER;
	st.st_y += YCENTER;
	st.st_z += ZCENTER;

	roll = 0.0;
	if (t > M_PI / 2.0 && t < 3 * M_PI / 2.0)
		roll = t - M_PI / 2.0;
	else if (t > 3 * M_PI / 2.0 && t < 5 * M_PI/2.0)
		roll = M_PI;
	else if (t > 5 * M_PI / 2.0 && t < 7 * M_PI / 2.0)
		roll = M_PI + t - 5 * M_PI / 2.0;

	while (roll > 0.1) {
		vec_rotate(&st.st_uv, &st.st_lv, 0.1);
		roll -= 0.1;
	}

	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += 0.010;
	if (t > 4 * M_PI) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_cubanoid_x(void)
{
	return (dx_cubanoid(DIM_X));
}

int
dx_cubanoid_y(void)
{
mark_add(&st.st_v);
	return (dx_cubanoid(DIM_Y));
}

int
dx_cubanoid_z(void)
{
	return (dx_cubanoid(DIM_Z));
}

int
dx_corkscrew(int dim)
{
	static double t;
	double a, b, c;
	struct fvec sv;
	int ret;

	ret = 0;
	switch (dim) {
	case DIM_X:
		a = CABHEIGHT / 4.0;
		b = ((NROWS * ROWDEPTH + (NROWS - 1) * ROWSPACE)) / 4.0;
		c = ROWWIDTH;
		break;
	case DIM_Y:
		a = ROWWIDTH;
		b = ROWDEPTH;
		c = CABHEIGHT;
		break;
	case DIM_Z:
		a = ROWWIDTH;
		b = CABHEIGHT;
		c = ROWDEPTH;
		break;
	}

	sv.fv_x = t;
	sv.fv_y = a * sin(t / c * 4.0 * M_PI);
	sv.fv_z = b * cos(t / c * 4.0 * M_PI);

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);

	switch (dim) {
	case DIM_X:
		st.st_x = t;
		st.st_y = YCENTER + sv.fv_y;
		st.st_z = ZCENTER + sv.fv_z;

		if (t == 0.0) {
			vec_set(&st.st_uv, 0.0, 1.0, 0.0);
			vec_set(&st.st_lv, 1.0, 0.0, 0.0);
		}
		break;
	case DIM_Y:
		st.st_x = XCENTER + sv.fv_y;
		st.st_y = t;
		st.st_z = ZCENTER + sv.fv_z;

		if (t == 0.0) {
			vec_set(&st.st_uv, 0.0, 0.0, 1.0);
			vec_set(&st.st_lv, 0.0, 1.0, 0.0);
		}
		break;
	case DIM_Z:
		st.st_x = XCENTER + sv.fv_y;
		st.st_y = YCENTER + sv.fv_z;
		st.st_z = t;

		if (t == 0.0) {
			vec_set(&st.st_uv, 0.0, 1.0, 0.0);
			vec_set(&st.st_lv, 0.0, 0.0, 1.0);
		}
		break;
	}

	vec_rotate(&st.st_uv, &st.st_lv, 2.0 * M_PI / c);
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += 0.5;
	if (t > c) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_corkscrew_x(void)
{
	return (dx_corkscrew(DIM_X));
}

int
dx_corkscrew_y(void)
{
	return (dx_corkscrew(DIM_Y));
}

int
dx_corkscrew_z(void)
{
	return (dx_corkscrew(DIM_Z));
}

int
dxp_bird(void)
{
	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	cam_bird();
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);
	return (1);
}

int
dxp_start(void)
{
	struct panel *p;
	int b;

	st.st_vmode = VM_PHYSICAL;
	st.st_dmode = DM_TEMP;
	st.st_rf |= RF_VMODE | RF_DMODE;

	b = OP_FRAMES | OP_TWEEN | OP_DISPLAY | OP_NODEANIM | OP_NLABELS;
	opt_disable(~b);
	opt_enable(b);

	b = PANEL_LEGEND | PANEL_VMODE | PANEL_DMODE | PANEL_HELP;
	TAILQ_FOREACH(p, &panels, p_link)
		if (b & p->p_id)
			b &= ~p->p_id;
		else
			b |= p->p_id;
	panels_flip(b);
	dxp_bird();
	return (1);
}

int
dxp_selnode(void)
{
	struct node *n;

	n = node_for_nid(0);
	sn_add(n, &fv_zero);
	panel_show(PANEL_NINFO);
	return (1);
}

int
dxp_refocus(void)
{
	tween_push(TWF_POS | TWF_UP | TWF_LOOK);
	cam_revolvefocus(0.0, 0.01);
	tween_pop(TWF_POS | TWF_UP | TWF_LOOK);
	return (1);
}

int
dxp_hlseldm(void)
{
	st.st_hlnc = HL_SELDM;
	st.st_rf |= RF_HLNC;
	return (1);
}

int
dxp_hlall(void)
{
	st.st_hlnc = HL_ALL;
	st.st_rf |= RF_HLNC;
	return (1);
}

int
dxp_seljob(void)
{
	struct node *n;
	struct ivec iv;

	if (job_list.ol_cur > 0) {
		NODE_FOREACH(n, &iv)
			if (n && n->n_job) {
				sn_add(n, &fv_zero);
				panel_show(PANEL_NINFO);
				dxp_hlseldm();
				return (1);
			}
	}
	return (1);
}

int
dxp_clrsn(void)
{
	sn_clear();
	panel_hide(PANEL_NINFO);
	return (1);
}

int
dxp_end(void)
{
	st.st_opts &= ~OP_DEUSEX;
	return (1);
}

int
dxp_op_skel(void)
{
	opt_enable(OP_SKEL);
	return (1);
}

int
dxp_nop_skel(void)
{
	opt_disable(OP_SKEL);
	return (1);
}

int
dxp_op_pipes(void)
{
	opt_enable(OP_PIPES);
	return (1);
}

int
dxp_nop_pipes(void)
{
	opt_disable(OP_PIPES);
	return (1);
}

int
dxp_wisnake(void)
{
	static int t;
	int ret;

	if ((t % 50) == 0) {
		st.st_wioff.iv_x++;
		st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | \
		    RF_NODESWIV;
	}

	ret = 0;
	if (++t >= 300) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_vm_phys(void)
{
	st.st_vmode = VM_PHYSICAL;
	st.st_rf |= RF_VMODE;
	return (1);
}

int
dxp_vm_wione(void)
{
	st.st_vmode = VM_WIREDONE;
	st.st_rf |= RF_VMODE;
	return (1);
}

int
dxp_dm_job(void)
{
	st.st_dmode = DM_JOB;
	st.st_rf |= RF_DMODE;
	return (1);
}

int
dxp_dm_temp(void)
{
	st.st_dmode = DM_TEMP;
	st.st_rf |= RF_DMODE;
	return (1);
}

int
dxp_panel_pipe(void)
{
	panel_show(PANEL_PIPE);
	return (1);
}

int
dxp_npanel_pipe(void)
{
	panel_hide(PANEL_PIPE);
	return (1);
}

int
dxp_camsync(void)
{
	return (memcmp(&st.st_v, &tv, sizeof(tv)) == 0);
}

int
dxp_stall(void)
{
	static int t;
	int ret;

	ret = 0;
	if (++t >= fps * 3) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_sstall(void)
{
	static int t;
	int ret;

	ret = 0;
	if (++t >= fps) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_forw(void)
{
	static int t;
	int ret;

	if ((t % 3) == 0) {
		tween_push(TWF_POS);
		cam_move(DIR_FORWARD, 0.5);
		tween_pop(TWF_POS);
	}

	ret = 0;
	if (++t >= 300) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

struct dxte {
	void (*de_init)(void);
	int  (*de_update)(void);
} dxtab[] = {
	{ NULL, dxp_start },
	{ NULL, dxp_camsync },
	{ NULL, dx_orbit_urev },
	{ NULL, dxp_camsync },
	{ NULL, dxp_selnode },
	{ NULL, dxp_hlseldm },
	{ NULL, dxp_op_skel },
	{ NULL, dxp_sstall },
	{ NULL, dxp_refocus },
	{ NULL, dxp_camsync },
	{ NULL, dxp_sstall },
	{ NULL, dxp_forw },
	{ NULL, dxp_camsync },
	{ NULL, dx_orbit_u },
	{ NULL, dxp_camsync },
	{ NULL, dxp_clrsn },
	{ NULL, dxp_bird },
	{ NULL, dxp_hlall },
	{ NULL, dxp_camsync },
	{ NULL, dxp_nop_skel },
	{ NULL, dxp_vm_wione },
	{ NULL, dxp_bird },
	{ NULL, dxp_camsync },
	{ NULL, dxp_dm_job },
	{ NULL, dxp_panel_pipe },
	{ NULL, dxp_stall },
	{ NULL, dx_orbit_urev },
	{ NULL, dxp_op_pipes },
	{ NULL, dxp_camsync },
	{ NULL, dxp_seljob },
	{ NULL, dxp_sstall },
	{ NULL, dxp_refocus },
	{ NULL, dxp_camsync },
	{ NULL, dx_orbit_u },
	{ NULL, dxp_hlall },
	{ NULL, dxp_clrsn },
	{ NULL, dxp_refocus },
	{ NULL, dxp_camsync },
	{ NULL, dxp_nop_pipes },
	{ NULL, dxp_npanel_pipe },
	{ NULL, dxp_sstall },
	{ NULL, dxp_wisnake },
	{ NULL, dxp_sstall },
	{ NULL, dxp_vm_phys },
	{ NULL, dxp_bird },
#if 0
	{ dxi_curlyq, dx_curlyq },
	{ dxi_orbit, dx_orbit_u },
	{ dxi_orbit, dx_orbit_urev },
	{ dxi_orbit, dx_orbit_v },
	{ dxi_orbit, dx_orbit_vrev },
	{ NULL, dx_cubanoid_y },
	{ dxi_orbit, dx_orbit3_u },
	{ dxi_orbit, dx_orbit3_urev },
	{ dxi_orbit, dx_orbit3_v },
	{ dxi_orbit, dx_orbit3_vrev },
	{ NULL, dx_corkscrew_x }
#endif
};
#define NENTRIES(t) (sizeof((t)) / sizeof((t)[0]))

#if 0
void
dx_update(void)
{
	static struct dxte *de;

	if (de == NULL) {
		de = &dxtab[rand() % NENTRIES(dxtab)];
		if (de->de_init != NULL)
			de->de_init();
	}

	if (de->de_update())
		de = NULL;
}
#endif

void
dx_update(void)
{
	static struct dxte *de;
	static size_t i;

	if (de == NULL) {
		de = &dxtab[i];
		if (++i >= NENTRIES(dxtab))
			i = 0;
	}
	if (de->de_update())
		de = NULL;
}
