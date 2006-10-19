/* $Id$ */

#include "mon.h"

#include <err.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "cam.h"
#include "deusex.h"
#include "dx-parse.h"
#include "env.h"
#include "lnseg.h"
#include "mark.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "panel.h"
#include "pathnames.h"
#include "queue.h"
#include "reel.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"
#include "util.h"
#include "xmath.h"

int		  dx_built;		/* Whether dx path is parsed. */
int		  dx_active;		/* Whether dx mode is running. */
struct dx_action *dx_action;		/* Current dx action. */
char		  dx_fn[PATH_MAX] = _PATH_DXSCRIPTS "/" DX_DEFAULT;
char		  dx_dir[PATH_MAX] = _PATH_DXSCRIPTS;
struct objlist	  dxscript_list = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };

int		  dx_cycle_nc = -1;
int		  dxt_done;
int		  dxt_set;

void
dxa_add(const struct dx_action *dxa)
{
	struct dx_action stall, *p;

	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	*p = *dxa;
	TAILQ_INSERT_TAIL(&dxlist, p, dxa_link);

	if (dxa->dxa_type != DGT_STALL) {
		memset(&stall, 0, sizeof(stall));
		stall.dxa_type = DGT_STALL;
		stall.dxa_stall_secs = 1.0;
		dxa_add(&stall);
	}
}

void
dxa_clear(void)
{
	struct dx_action *dxa;

	while ((dxa = TAILQ_FIRST(&dxlist)) != TAILQ_END(&dxlist)) {
		free(dxa->dxa_str);
		TAILQ_REMOVE(&dxlist, dxa, dxa_link);
		free(dxa);
	}
	TAILQ_INIT(&dxlist);
}

char *
dx_set(const char *fn, int flags)
{
	if ((flags & CHF_DIR) == 0) {
		dx_built = 0;
		snprintf(dx_fn, sizeof(dx_fn), "%s/%s", dx_dir, fn);
	}
	panel_rebuild(PANEL_DXCHO);
	return (dx_dir);
}

int
dxp_orbit(const struct dx_action *dxa)
{
	static double amt, adj;
	static int t, sample;
	double wait, du, dv, max;
	int ret;

	/* Get a good FPS sample before we start. */
	if (++sample < fps)
		return (0);

	max = 2 * M_PI * dxa->dxa_orbit_frac;
	if (t == 0) {
		if (dxa->dxa_orbit_secs)
			wait = dxa->dxa_orbit_secs;
		else
			wait = 1.1 * ceil(log(DST(&st.st_v, &focus) /
			    log(1.1)));
		adj = 2 * M_PI / (fps * wait);
		amt = 0.0;
	}

	du = dv = 0.0;
	switch (dxa->dxa_orbit_dim) {
	case DIM_X:
	case DIM_Z:
		du = dxa->dxa_orbit_dir * adj;
		break;
	case DIM_Y:
		dv = dxa->dxa_orbit_dir * -adj;
		break;
	}

	amt += fabs(adj);

	tween_push(TWF_POS | TWF_LOOK | TWF_UP);
	cam_revolvefocus(du, dv, REVT_LKAVG);
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
		sample = 0;
	}
	return (ret);
}

int
dxp_curlyq(const struct dx_action *dxa)
{
	static double fwadj;
	static int t;
	double wait;
	int ret;

	if (t == 0) {
		if (dxa->dxa_orbit_secs)
			wait = dxa->dxa_orbit_secs;
		else
			wait = 1.1 * ceil(log(DST(&st.st_v, &focus) /
			    log(1.1)));
		fwadj = DST(&st.st_v, &focus) / (fps * wait);
	}

	tween_push(TWF_POS | TWF_LOOK | TWF_UP);
	cam_move(DIR_FORW, fwadj);
	tween_pop(TWF_POS | TWF_LOOK | TWF_UP);

	ret = 0;
	if (dxp_orbit(dxa)) {
		ret = 1;
		t = 0;
	}
	return (ret);
}

int
dxp_cuban8(const struct dx_action *dxa)
{
	static double t;

	struct fvec sv, uv, lv, xv, axis;
	double roll, a, b, max;
	float mag;
	int ret;

	a = b = 0.0; /* gcc */
	switch (dxa->dxa_cuban8_dim) {
	case DIM_X:
		a = CABHEIGHT / 4.0;
		b = ROWDEPTH / 4.0;
		break;
	case DIM_Y:
		a = ROWWIDTH / 4.0;
		b = CABHEIGHT / 2.0;
		break;
	case DIM_Z:
		a = ROWWIDTH / 4.0 - CABWIDTH;
		b = CABHEIGHT / 4.0;
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

	switch (dxa->dxa_cuban8_dim) {
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
dxp_corkscrew(const struct dx_action *dxa)
{
	static double t;
	double a, b, c;
	struct fvec sv;
	int ret;

	ret = 0;
	a = b = c = 0.0; /* gcc */
	switch (dxa->dxa_screw_dim) {
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

	switch (dxa->dxa_screw_dim) {
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
dxp_bird(__unused const struct dx_action *dxa)
{
	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	cam_bird();
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);
	return (1);
}

void
dxpcb_playreel(__unused int a)
{
	dxt_done = 1;
	reel_advance();
}

int
dxp_playreel(const struct dx_action *dxa)
{
	static size_t save_reel_pos;
	static int loaded;
	int ret;

	if (!loaded) {
		snprintf(reel_browsedir, PATH_MAX,
		    "%s", smart_dirname(dxa->dxa_reel));
		if (reel_set(smart_basename(dxa->dxa_reel), CHF_DIR))
			return (1);
		reel_load();
		if (reelframe_list.ol_cur == 0)
			return (1);
		reel_start();
		loaded = 1;
		opt_enable(OP_REEL);
	}

	if (reelframe_list.ol_cur == 0)
		return (1);

	ret = 0;
	if (dxt_done) {
		dxt_done = 0;
		dxt_set = 0;
		if (save_reel_pos == reel_pos) {
			reel_end();
			opt_disable(OP_REEL);
			ret = 1;
			loaded = 0;
		}
		save_reel_pos = reel_pos;
	}
	if (!ret && !dxt_set) {
		glutTimerFunc(dxa->dxa_reel_delay, dxpcb_playreel, 0);
		dxt_set = 1;
	}
	return (ret);
}

int
dxp_refocus(__unused const struct dx_action *dxa)
{
	tween_push(TWF_POS | TWF_UP | TWF_LOOK);
	cam_revolvefocus(0.0, 0.001, REVT_LKAVG);
	tween_pop(TWF_POS | TWF_UP | TWF_LOOK);
	return (1);
}

int
dxp_refresh(__unused const struct dx_action *dxa)
{
	static const char *cap;
	int ret;

	ret = 0;
	if (cap) {
		ret = 1;
		caption_set(cap);
		cap = NULL;
	} else {
		cap = caption_get();
		st.st_rf |= RF_DATASRC;
	}
	return (ret);
}

int
dxp_selnode(const struct dx_action *dxa)
{
	struct node *n;

	switch (dxa->dxa_selnode) {
	case DXSN_VIS:
		/* XXX */
		break;
	case DXSN_RND:
		do {
			n = node_for_nid(random() % 1000);
		} while (n == NULL);
		sn_add(n, &fv_zero);
		panel_show(PANEL_NINFO);
		break;
	default:
		if ((n = node_for_nid(dxa->dxa_selnode)) != NULL) {
			sn_add(n, &fv_zero);
			panel_show(PANEL_NINFO);
		}
	}
	return (1);
}

int
dxp_subsel(const struct dx_action *dxa)
{
	struct node *n;
	struct ivec iv;

	switch (dxa->dxa_subselnode) {
	case DXSN_RND:
		/* XXX */
		break;
	case DXSN_VIS:
		NODE_FOREACH(n, &iv)
			if (n) {
				if (n->n_fillp->f_a)
					n->n_flags |= NF_SHOW;
				else
					n->n_flags &= ~NF_SHOW;
			}
		break;
	default:
		/* XXX */
		break;
	}
	return (1);
}

int
dxp_hl(const struct dx_action *dxa)
{
	nc_set(dxa->dxa_hl);
	return (1);
}

int
dxp_seljob(const struct dx_action *dxa)
{
	struct node *n;
//	struct job *j;

	if (dxa->dxa_seljob == DXSJ_RND) {
		if (job_list.ol_cur > 0) {
			do {
				n = node_for_nid(random() % NID_MAX);
			} while (n == NULL || n->n_job == NULL);
			sn_add(n, &fv_zero);
			panel_show(PANEL_NINFO);
			nc_runall(fill_setxparent);
			nc_runsn(fill_setopaque);
		}
//	} else if (j = job_findbyid(dxa->dxa_seljob)) {
//		dxp_hlnc(NC_SELDM);
	}
	return (1);
}

int
dxp_clrsn(__unused const struct dx_action *dxa)
{
	sn_clear();
	panel_hide(PANEL_NINFO);
	return (1);
}

int
dxp_opt(const struct dx_action *dxa)
{
	switch (dxa->dxa_opt_mode) {
	case DXV_ON:
		opt_enable(dxa->dxa_opts);
		break;
	case DXV_OFF:
		opt_disable(dxa->dxa_opts);
		break;
	case DXV_SET:
		opt_flip(st.st_opts ^ dxa->dxa_opts);
		break;
	}
	return (1);
}

int
dxp_vmode(const struct dx_action *dxa)
{
	st.st_vmode = dxa->dxa_vmode;
	st.st_rf |= RF_VMODE;
	return (1);
}

int
dxp_dmode(const struct dx_action *dxa)
{
	st.st_dmode = dxa->dxa_dmode;
	st.st_rf |= RF_DMODE;
	return (1);
}

int
dxp_panel(const struct dx_action *dxa)
{
	switch (dxa->dxa_panel_mode) {
	case DXV_ON:
		panels_show(dxa->dxa_panels);
		break;
	case DXV_OFF:
		panels_hide(dxa->dxa_panels);
		break;
	case DXV_SET:
		panels_set(dxa->dxa_panels);
		break;
	}
	return (1);
}

int
dxp_pipemode(const struct dx_action *dxa)
{
	st.st_pipemode = dxa->dxa_pipemode;
	st.st_rf |= RF_CLUSTER | RF_SELNODE;
	return (1);
}

int
dxp_pstick(const struct dx_action *dxa)
{
	int pids, b;

	pids = dxa->dxa_panels;
	while (pids) {
		b = ffs(pids) - 1;
		pids &= ~(1 << b);
		pinfo[b].pi_stick = dxa->dxa_pstick;
	}
	panel_redraw(dxa->dxa_panels);
	return (1);
}

int
dxp_camsync(__unused const struct dx_action *dxa)
{
	if (st.st_opts & OP_TWEEN)
		return (DST(&st.st_v, &tv) < 0.1);
	else
		return (1);
}

void
dxpcb_stall(__unused int a)
{
	dxt_done = 1;
}

int
dxp_stall(const struct dx_action *dxa)
{
	int ret;

	ret = 0;
	if (dxt_done) {
		dxt_done = 0;
		dxt_set = 0;
		ret = 1;
	}
	if (!ret && !dxt_set) {
		glutTimerFunc(dxa->dxa_stall_secs * 1000, dxpcb_stall, 0);
		dxt_set = 1;
	}
	return (ret);
}

int
dxp_nodesync(__unused const struct dx_action *dxa)
{
	return ((st.st_rf & RF_CLUSTER) == 0);
}

int
dxp_winsp(const struct dx_action *dxa)
{
	static struct ivec iv;
	static int t;
	int ret, j;

	if (t == 0)
		iv = dxa->dxa_winsp;

//	if ((t % 2) == 0)
		for (j = 0; j < NDIM; j++)
			if (iv.iv_val[j]) {
				switch (dxa->dxa_winsp_mode.iv_val[j]) {
				case DXV_ON:
					st.st_winsp.iv_val[j]++;
					iv.iv_val[j]--;
					break;
				case DXV_OFF:
					st.st_winsp.iv_val[j]--;
					iv.iv_val[j]--;
					break;
				case DXV_SET:
					if (st.st_winsp.iv_val[j] < iv.iv_val[j])
						st.st_winsp.iv_val[j]++;
					else if (st.st_winsp.iv_val[j] > iv.iv_val[j])
						st.st_winsp.iv_val[j]--;
					else
						iv.iv_val[j] = 0;
					break;
				}
				st.st_rf |= RF_CLUSTER | RF_SELNODE | \
				    RF_GROUND | RF_NODESWIV | RF_CAM;
			}

	t++;
	ret = 0;
	if (iv.iv_x == 0 &&
	    iv.iv_y == 0 &&
	    iv.iv_z == 0) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_wioff(const struct dx_action *dxa)
{
	static struct ivec iv;
	static int t;
	int ret, j;

	if (t == 0)
		iv = dxa->dxa_wioff;

	if ((t % 10) == 0)
		for (j = 0; j < NDIM; j++)
			if (iv.iv_val[j]) {
				switch (dxa->dxa_wioff_mode.iv_val[j]) {
				case DXV_ON:
					st.st_wioff.iv_val[j]++;
					iv.iv_val[j]--;
					break;
				case DXV_OFF:
					st.st_wioff.iv_val[j]--;
					iv.iv_val[j]--;
					break;
				case DXV_SET:
					if (st.st_wioff.iv_val[j] < iv.iv_val[j])
						st.st_wioff.iv_val[j]++;
					else if (st.st_wioff.iv_val[j] > iv.iv_val[j])
						st.st_wioff.iv_val[j]--;
					else
						iv.iv_val[j] = 0;
					break;
				}
				st.st_rf |= RF_CLUSTER | RF_SELNODE | \
				    RF_GROUND | RF_NODESWIV;
				dxp_refocus(NULL);
			}

	t++;
	ret = 0;
	if (iv.iv_x == 0 &&
	    iv.iv_y == 0 &&
	    iv.iv_z == 0) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dxp_cam_move(const struct dx_action *dxa)
{
	static double amt, adj;
	static int t;
	int ret;

	if (adj == 0.0)
		adj = dxa->dxa_move_amt / (dxa->dxa_move_secs * fps);

	tween_push(TWF_POS);
	cam_move(dxa->dxa_move_dir, adj);
	tween_pop(TWF_POS);

	amt += adj;

	ret = 0;
	t++;
	if (amt >= dxa->dxa_move_amt) {
		t = 0;
		ret = 1;
		amt = 0.0;
		adj = 0.0;
	}
	return (ret);
}

void
dxpcb_cyclenc(__unused int a)
{
	dx_cycle_nc++;
	dxt_done = 1;
}

int
dxp_cyclenc(const struct dx_action *dxa)
{
	int ret, error;

	ret = 0;
	if (dxt_done) {
		if (dx_cycle_nc == 0)
			switch (dxa->dxa_cycle_meth) {
			case DACM_GROW:
				nc_runall(fill_setxparent);
				break;
			}

		switch (st.st_dmode) {
		case DM_JOB:
			while (dx_cycle_nc < NSC &&
			    statusclass[dx_cycle_nc].nc_nmemb == 0)
				dx_cycle_nc++;
			break;
		case DM_TEMP:
			while (dx_cycle_nc < NTEMPC &&
			    tempclass[dx_cycle_nc].nc_nmemb == 0)
				dx_cycle_nc++;
			break;
		}

		error = 1;
		switch (dxa->dxa_cycle_meth) {
		case DACM_CYCLE:
			error = nc_set(dx_cycle_nc);
			break;
		case DACM_GROW:
			error = nc_apply(fill_setopaque, dx_cycle_nc);
			break;
		}
		if (error) {
			dx_cycle_nc = -1;
			nc_runall(fill_setopaque);
			ret = 1;
		}
		dxt_done = 0;
		dxt_set = 0;
	}
	if (!ret && !dxt_set) {
		glutTimerFunc(1000, dxpcb_cyclenc, 0);
		dxt_set = 1;
	}
	return (ret);
}

int
dxp_caption(const struct dx_action *dxa)
{
	caption_set(dxa->dxa_caption);
	return (1);
}

int
dxp_exit(__unused const struct dx_action *dxa)
{
	exit(0);
}

struct dxent {
	int	  de_val;
	int	(*de_update)(const struct dx_action *);
} dxtab[] = {
	{ DGT_BIRD,	dxp_bird },
	{ DGT_CAMSYNC,	dxp_camsync },
	{ DGT_CLRSN,	dxp_clrsn },
	{ DGT_CORKSCREW,dxp_corkscrew },
	{ DGT_CUBAN8,	dxp_cuban8 },
	{ DGT_CURLYQ,	dxp_curlyq },
	{ DGT_CYCLENC,	dxp_cyclenc },
	{ DGT_DMODE,	dxp_dmode },
	{ DGT_EXIT,	dxp_exit },
	{ DGT_HL,	dxp_hl },
	{ DGT_MOVE,	dxp_cam_move },
	{ DGT_NODESYNC,	dxp_nodesync },
	{ DGT_OPT,	dxp_opt },
	{ DGT_ORBIT,	dxp_orbit },
	{ DGT_PANEL,	dxp_panel },
	{ DGT_PIPEMODE,	dxp_pipemode },
	{ DGT_PSTICK,	dxp_pstick },
	{ DGT_PLAYREEL,	dxp_playreel },
	{ DGT_REFOCUS,	dxp_refocus },
	{ DGT_REFRESH,	dxp_refresh },
	{ DGT_SELJOB,	dxp_seljob },
	{ DGT_SELNODE,	dxp_selnode },
	{ DGT_SETCAP,	dxp_caption },
	{ DGT_STALL,	dxp_stall },
	{ DGT_SUBSEL,	dxp_subsel },
	{ DGT_VMODE,	dxp_vmode },
	{ DGT_WINSP,	dxp_winsp },
	{ DGT_WIOFF,	dxp_wioff }
};

void
dx_update(void)
{
	static struct dxent *de;
	size_t j;

	if (st.st_opts & OP_STOP)
		return;

	if (dx_action == NULL) {
		dx_action = TAILQ_FIRST(&dxlist);
		if (dx_action == NULL)
//			status_add();
			return;
		de = NULL;

		//if (st.st_opts & )
	}

	if (de == NULL) {
		for (j = 0; j < NENTRIES(dxtab); j++)
			if (dxtab[j].de_val == dx_action->dxa_type)
				break;
		if (j >= NENTRIES(dxtab))
			errx(1, "internal error: unknown dx type %d",
			    dx_action->dxa_type);
		de = &dxtab[j];
	}

	if (de->de_update(dx_action)) {
		dx_action = TAILQ_NEXT(dx_action, dxa_link);
		de = NULL;
	}
}

struct state dx_save_state;
const char *oldcap;

void
dx_start(void)
{
	dx_active = 1;
	if (!dx_built)
		dx_parse();
	dx_action = NULL;
	dx_save_state = st;
	oldcap = caption_get();
}

void
dx_end(void)
{
	dx_active = 0;
	load_state(&dx_save_state);
	caption_set(oldcap);
}
