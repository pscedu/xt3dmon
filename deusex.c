/* $Id$ */

#include "mon.h"

#include <err.h>
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
#include "nodeclass.h"
#include "objlist.h"
#include "panel.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"

#define DST(a, b)					\
	(sqrt(						\
	    SQUARE((a)->fv_x - (b)->fv_x) +		\
	    SQUARE((a)->fv_y - (b)->fv_y) +		\
	    SQUARE((a)->fv_z - (b)->fv_z)))

TAILQ_HEAD(, dx_action) dxalist;

void
dxa_add(struct dx_action *dxa)
{
	struct dx_action *p;

	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	*p = *dxa;
	TAILQ_INSERT_TAIL(&dxalist, p, dxa_link);
}

void
dxi_orbit(void)
{
	double rx, ry, rz;

	tween_push(TWF_POS);
	do {
		rx = (random() % 2 ? 1 : -1) * fmod(random() / 100.0, ROWWIDTH);
		ry = (random() % 2 ? 1 : -1) * fmod(random() / 100.0, ROWWIDTH);
		rz = (random() % 2 ? 1 : -1) * fmod(random() / 100.0, ROWWIDTH);

		st.st_v = focus;
		st.st_x += rx;
		st.st_y += ry;
		st.st_z += rz;
	} while (DST(&focus, &st.st_v) < CL_DEPTH);
	tween_pop(TWF_POS);
}

int
dxp_orbit(int dim, int sign)
{
	static double amt, adj;
	static int t;
	double wait, du, dv, max;
	int ret;

	max = 2 * M_PI;
	if (t == 0) {
		wait = 1.1 * ceil(log(DST(&st.st_v, &focus) / log(1.1)));
		adj = max / (fps * wait);
		amt = 0.0;
	}

	du = dv = 0.0;
	switch (dim) {
	case DIM_X:
	case DIM_Z:
		du = sign * adj;
		break;
	case DIM_Y:
		dv = sign * -adj;
		break;
	}

	amt += fabs(adj);

	tween_push(TWF_POS | TWF_LOOK | TWF_UP);
	cam_revolvefocus(du, dv);
	tween_pop(TWF_POS | TWF_LOOK | TWF_UP);

	/*
	 * Add a small skew amount here (0.001)
	 * to account for FP rounding.
	 */
	ret = 0;
	t++;
	if (amt + 0.001 >= max) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_orbit3(int dim, int sign)
{
	static int n = 0;
	int ret;

	if (dxp_orbit(dim, sign))
		n++;

	ret = 0;
	if (n >= 3) {
		n = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_curlyq(void)
{
	static double fwadj, adj, amt;
	static int t;
	int ret, dim, sign;
	double max, du, dv, wait;

	max = 3 * 2 * M_PI;
	if (t == 0) {
		wait = 1.1 * ceil(log(DST(&st.st_v, &focus) / log(1.1)));
		fwadj = DST(&st.st_v, &focus) / (fps * wait);
		adj = max / (fps * wait);
		amt = 0.0;
	}

	sign = 1;
	dim = DIM_X;

	du = dv = 0.0;
	switch (dim) {
	case DIM_X:
		du = sign * adj;
		break;
	case DIM_Y:
		dv = sign * -adj;
		break;
	}

	amt += fabs(adj);

	tween_push(TWF_POS | TWF_LOOK | TWF_UP);
	cam_move(DIR_FORW, fwadj);
	cam_revolvefocus(du, dv);
	tween_pop(TWF_POS | TWF_LOOK | TWF_UP);

	ret = 0;
	t++;
	if (amt >= max) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_cuban8(int dim)
{
	static double t;
	struct fvec sv, uv, lv, xv, axis;
	double roll, a, b, max;
	float mag;
	int ret;

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
	vec_rotate(&st.st_v, &axis, atan2(CL_DEPTH / 2.0, CL_WIDTH));
#endif

	mag = vec_mag(&st.st_v);
	vec_set(&axis, 1.0, 0.0, 0.0);
	vec_rotate(&st.st_v, &axis, M_PI / 2.0);
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

//	dxp_refocus();

	ret = 0;
	t += 0.015;
	if (t > 4 * M_PI) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_corkscrew(int dim)
{
	static double t;
	double a, b, c;
	struct fvec sv;
	int ret;

	ret = 0;
	switch (dim) {
	case DIM_X:
		a = CABHEIGHT / 4.0;
		b = CL_DEPTH / 4.0;
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
dxp_bird(void)
{
	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	cam_bird();
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);
	return (1);
}

int
dxp_refocus(void)
{
	tween_push(TWF_POS | TWF_UP | TWF_LOOK);
	cam_revolvefocus(0.0, 0.001);
	tween_pop(TWF_POS | TWF_UP | TWF_LOOK);
	return (1);
}

int
dxp_start(void)
{
	struct panel *p;
	int b;

	st.st_vmode = VM_PHYS;
	st.st_dmode = DM_TEMP;
	st.st_rf |= RF_VMODE | RF_DMODE | RF_DATASRC;
	st.st_pipemode = PM_DIR;

	b = OP_FRAMES | OP_TWEEN | OP_DISPLAY | OP_NODEANIM | OP_CAPTION;
	if (st.st_opts & OP_DEUSEX)
		b |= OP_DEUSEX;
	opt_disable(~b);
	opt_enable(b);

	b = PANEL_LEGEND | PANEL_HELP;
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
dxp_selnode_rnd(void)
{
	struct node *n;

	do {
		n = node_for_nid(random() % 1000);
	} while (n == NULL);

	sn_add(n, &fv_zero);
	panel_show(PANEL_NINFO);
	return (1);
}

int
dxp_selnode(int nid)
{
	struct node *n;

	n = node_for_nid(nid);
	if (n == NULL)
		errx(1, "nid %d is NULL", nid);
	sn_add(n, &fv_zero);
	panel_show(PANEL_NINFO);
	return (1);
}

int
dxp_hlnc(int nc)
{
	st.st_hlnc = nc;
	st.st_rf |= RF_HLNC;
	return (1);
}

int
dxp_seljob(void)
{
	struct node *n;

	if (job_list.ol_cur > 0) {
		do {
			n = node_for_nid(random() % NID_MAX);
		} while (n == NULL || n->n_job == NULL);
		sn_add(n, &fv_zero);
		panel_show(PANEL_NINFO);
		dxp_hlnc(HL_SELDM);
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
dxp_op(int opt)
{
	opt_enable(opt);
	return (1);
}

int
dxp_nop(int opt)
{
	opt_disable(opt);
	return (1);
}

int
dxp_widepuff(void)
{
	st.st_winsp.iv_x -= 10;
	st.st_winsp.iv_y -= 10;
	st.st_winsp.iv_z -= 10;
	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | \
	    RF_NODESWIV | RF_CAM;
	return (1);
}

int
dxp_wipuff(void)
{
	st.st_winsp.iv_x += 10;
	st.st_winsp.iv_y += 10;
	st.st_winsp.iv_z += 10;
	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | \
	    RF_NODESWIV | RF_CAM;
	return (1);
}

int
dxp_vmode(int vmode)
{
	st.st_vmode = vmode;
	st.st_rf |= RF_VMODE;
	return (1);
}

int
dxp_dmode(int dmode)
{
	st.st_dmode = dmode;
	st.st_rf |= RF_DMODE;
	return (1);
}

int
dxp_panel(int panel)
{
	panel_show(panel);
	return (1);
}

int
dxp_npanel(int panel)
{
	panel_hide(panel);
	return (1);
}

int
dxp_camsync(void)
{
	if (st.st_opts & OP_TWEEN)
		return (DST(&st.st_v, &tv) < 0.1);
	else
		return (1);
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
	static int t, ofps;
	int ret;

	if (ofps == 0)
		ofps = fps;

	ret = 0;
	t++;
	if (t >= 2 * fps ||
	    (t >= fps && fps != ofps)) {
		t = 0;
		ret = 1;
		ofps = 0;
	}
	return (ret);
}

int
dxp_wisnake(void)
{
	static int t, lvl;
	int max, wait, ret;
	struct ivec wioff;

	max = 64;
	wait = 8;
	if ((t % wait) == 0) {
		wioff = st.st_wioff;
		if (t > wait * 7 * max / 8.0 - 1)
			;
		else if (t > wait * 6 * max / 8.0 - 1)
			st.st_wioff.iv_z--;
		else if (t > wait * 5 * max / 8.0 - 1)
			;
		else if (t > wait * 4 * max / 8.0 - 1)
			st.st_wioff.iv_x--;
		else if (t > wait * 3 * max / 8.0 - 1) {
			if (t == wait * 3 * max / 8.0) {
				t--;
				if (lvl < 1) {
				 	if (dxp_camsync())
						lvl++;
				} else if (lvl < 2) {
				 	if (dxp_sstall())
						lvl++;
				} else if (lvl < 3) {
				 	if (dxp_orbit(DIM_X, -1))
						lvl++;
				} else if (lvl < 4) {
				 	if (dxp_camsync())
						lvl++;
				} else if (lvl < 5) {
				 	if (dxp_sstall())
						lvl++;
				} else
					t++;
			}
		} else if (t > wait * 2 * max / 8.0 - 1)
			st.st_wioff.iv_z++;
		else if (t > wait * 1 * max / 8.0 - 1)
			;
		else
			st.st_wioff.iv_x++;

		if (memcmp(&st.st_wioff, &wioff, sizeof(wioff)) != 0) {
			st.st_rf |= RF_CLUSTER | RF_SELNODE | \
			    RF_GROUND | RF_NODESWIV;
			dxp_refocus();
		}
	}

	ret = 0;
	if (++t >= wait * max) {
		t = 0;
		lvl = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_cam_move(double max, int wait, int dir)
{
	static int t;
	static double amt, adj;
	int ret;

	if (adj == 0.0)
		adj = max / (wait * fps);

	tween_push(TWF_POS);
	cam_move(dir, adj);
	tween_pop(TWF_POS);

	amt += adj;

	ret = 0;
	t++;
	if (amt >= max) {
		t = 0;
		ret = 1;
		amt = 0.0;
		adj = 0.0;
	}
	return (ret);
}

int
dxp_cam_move(int dir)
{
	return (dx_cam_move(50.0, 3, dir));
}

int
dxp_nodesync(void)
{
	return ((st.st_rf & RF_CLUSTER) == 0);
}

int
dxp_cyclenc(void)
{
	static int t, max, nnc, ncp;
	int ret, j;

	if (t == 0) {
		nnc = 0;
		switch (st.st_dmode) {
		case DM_JOB:
			nnc = job_list.ol_cur;
			for (j = 0; j < NSC; j++)
				if (statusclass[j].nc_nmemb)
					nnc++;
			break;
		case DM_TEMP:
			for (j = 0; j < NTEMPC; j++)
				if (tempclass[j].nc_nmemb)
					nnc++;
			break;
		}

		max = nnc * 3 * fps;
		ncp = -1;

		/* -1 needed for DM_TEMP loop below. */
		st.st_hlnc = -1;
	}

	if (ncp != t * nnc / max) {
		ncp = t * nnc / max;
		switch (st.st_dmode) {
		case DM_JOB:
			for (j = st.st_hlnc + 1; j < NSC; j++)
				if (statusclass[j].nc_nmemb) {
					st.st_hlnc = j;
					break;
				}
			if (j >= NSC) {
				st.st_hlnc = 0;
				for (j = 0; j < NSC; j++)
					if (statusclass[j].nc_nmemb == 0)
						st.st_hlnc++;
				st.st_hlnc += ncp;
			}
			break;
		case DM_TEMP:
			for (j = st.st_hlnc + 1; j < NTEMPC; j++)
				if (tempclass[j].nc_nmemb) {
					st.st_hlnc = j;
					break;
				}
			break;
		}
		st.st_rf |= RF_HLNC;
	}

	ret = 0;
	if (++t >= max) {
		t = 0;
		ret = 1;
		st.st_hlnc = HL_ALL;
		st.st_rf |= RF_HLNC;
	}
	return (ret);
}

int
dxp_setcap(char *s)
{
	caption_set(s);
	return (1);
}

int
dxp_cam_move_focus(void)
{
	int ret;

	dxp_refocus();
	tween_push(TWF_POS);
	cam_move(DIR_FORW, 0.5);
	tween_pop(TWF_POS);

	ret = 0;
	if (DST(&st.st_v, &focus) < 0.1)
		ret = 1;
	return (ret);
}

#define DA_NIL  0
#define DA_STR  1
#define DA_DBL  2
#define DA_INT  3
#define DA_INT2 4

#define DEN(name)		{ (name), DA_NIL,  0.0,   0,     0,      NULL  }
#define DES(name, str)		{ (name), DA_STR,  0.0,   0,     0,      (str) }
#define DEB(name, dbl)		{ (name), DA_DBL,  (dbl), 0,     0,      NULL  }
#define DEI(name, itg)		{ (name), DA_INT,  0.0,   (itg), 0,      NULL  }
#define DEII(name, itg, itg2)	{ (name), DA_INT2, 0.0,   (itg), (itg2), NULL  }

struct dxte {
	void	 *de_update;
	int	  de_type;
	double	  de_arg_dbl;
	int	  de_arg_int;
	int	  de_arg_int2;
	char	 *de_arg_str;
} dxtab[] = {
	DEN(dxp_start),
	DEN(dxp_refocus),
	DES(dxp_setcap, "CPU temperatures in physical machine layout"),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEII(dxp_orbit, DIM_X, -1),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEN(dxp_selnode_rnd),
	DEI(dxp_hlnc, HL_SELDM),
	DEI(dxp_op, OP_SKEL),
	DES(dxp_setcap, "Zoom while viewing nodes in temperature range"),
	DEN(dxp_sstall),
	DEN(dxp_refocus),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_cam_move, DIR_FORW),
	DEI(dxp_op, OP_NLABELS),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEII(dxp_orbit, DIM_X, 1),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_nop, OP_NLABELS),
	DEN(dxp_clrsn),
	DEN(dxp_sstall),
	DEI(dxp_cam_move, DIR_BACK),
	DEN(dxp_bird),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_hlnc, HL_ALL),
	DES(dxp_setcap, "Cycle through all temperature ranges"),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEN(dxp_cyclenc),
	DEI(dxp_nop, OP_SKEL),
	DEN(dxp_sstall),
	DES(dxp_setcap, "Changing to \"wired\" mode"),
	DEN(dxp_sstall),
	DEI(dxp_vmode, VM_WIONE),
	DEN(dxp_nodesync),
	DEN(dxp_bird),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_dmode, DM_JOB),
	DES(dxp_setcap, "Jobs in wired mode (virtual interconnect)"),
	DEN(dxp_sstall),
	DEII(dxp_orbit, DIM_X, -1),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEN(dxp_seljob),
	DES(dxp_setcap, "Viewing one job in wired mode"),
	DEN(dxp_sstall),
	DEN(dxp_refocus),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_op, OP_SKEL),
	DEI(dxp_op, OP_NLABELS),
	DEN(dxp_sstall),
	DEI(dxp_cam_move, DIR_FORW),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEII(dxp_orbit, DIM_X, 1),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_cam_move, DIR_BACK),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_nop, OP_SKEL),
	DEI(dxp_nop, OP_NLABELS),
	DEI(dxp_hlnc, HL_ALL),
	DEN(dxp_clrsn),
	DES(dxp_setcap, "Preparing to zoom inside the interconnect"),
	DEN(dxp_sstall),
	DEN(dxp_refocus),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_panel, PANEL_CMP),
	DEI(dxp_op, OP_PIPES),
	DEN(dxp_wipuff),
	DEN(dxp_nodesync),
	DEN(dxp_sstall),
	DEI(dxp_selnode, 2600),
	DEN(dxp_refocus),
	DEN(dxp_camsync),
	DEN(dxp_stall),
	DEI(dxp_cam_move, DIR_FORW),
	DEI(dxp_cam_move, DIR_FORW),
	DEI(dxp_op, OP_NLABELS),
	DES(dxp_setcap, "Focus on one node"),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEII(dxp_orbit, DIM_X, 1),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEI(dxp_cam_move, DIR_BACK),
	DEI(dxp_nop, OP_NLABELS),
	DEN(dxp_clrsn),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DEN(dxp_bird),
	DEN(dxp_camsync),
	DEN(dxp_sstall),
	DES(dxp_setcap, "Change draw offset to show torus repetitions"),
	DEI(dxp_npanel, PANEL_CMP),
	DEN(dxp_widepuff),
	DEN(dxp_nodesync),
	DEN(dxp_sstall),
	DEI(dxp_panel, PANEL_WIADJ),
	DEN(dxp_sstall),
	DEN(dxp_wisnake),
	DEI(dxp_nop, OP_PIPES),
	DEN(dxp_sstall),
	DEI(dxp_npanel, PANEL_WIADJ),
	DEN(dxp_sstall),
	DEN(dxp_seljob),
	DEI(dxp_op, OP_SKEL),
	DES(dxp_setcap, "Viewing job between physical and wired modes"),
	DEN(dxp_stall),
	DEI(dxp_vmode, VM_PHYS),
	DEN(dxp_nodesync),
	DEN(dxp_stall),
	DEN(dxp_bird),
	DEN(dxp_camsync),
	DEN(dxp_stall),
	DEI(dxp_vmode, VM_WIONE),
	DEN(dxp_sstall),
	DEN(dxp_bird),
	DEN(dxp_camsync),
	DEN(dxp_stall),
	DEN(dxp_clrsn),
	DEI(dxp_nop, OP_SKEL),
	DEI(dxp_hlnc, HL_ALL),
	DES(dxp_setcap, "Going back to physical layout mode viewing jobs"),
	DEN(dxp_stall),
	DEI(dxp_vmode, VM_PHYS),
	DEN(dxp_nodesync),
	DEN(dxp_sstall),
	DEN(dxp_bird),
	DEN(dxp_camsync),
	DEN(dxp_stall),
	DES(dxp_setcap, "Cycle through all node states/jobs"),
	DEN(dxp_sstall),
	DEI(dxp_op, OP_SKEL),
	DEN(dxp_cyclenc),
	DEI(dxp_nop, OP_SKEL),
	DEN(dxp_stall),
};

#define NENTRIES(t) (sizeof((t)) / sizeof((t)[0]))

void
dx_update(void)
{
	static struct dxte *de;
	static size_t i;
	int (*fn)(void);
	int (*fs)(char *);
	int (*fd)(double);
	int (*fi)(int);
	int (*fi2)(int, int);

	if (de == NULL) {
		de = &dxtab[i];
		if (++i >= NENTRIES(dxtab))
			i = 0;
	}
	if (st.st_opts & OP_STOP)
		return;
	switch (de->de_type) {
	case DA_NIL:
		fn = de->de_update;
		if (fn())
			de = NULL;
		break;
	case DA_STR:
		fs = de->de_update;
		if (fs(de->de_arg_str))
			de = NULL;
		break;
	case DA_DBL:
		fd = de->de_update;
		if (fd(de->de_arg_dbl))
			de = NULL;
		break;
	case DA_INT:
		fi = de->de_update;
		if (fi(de->de_arg_int))
			de = NULL;
		break;
	case DA_INT2:
		fi2 = de->de_update;
		if (fi2(de->de_arg_int, de->de_arg_int2))
			de = NULL;
		break;
	}
}
