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

/*
 * Camera routines.
 * These functions all deal directly with the current
 * camera position, so make sure you surround any calls
 * to them with tween_push/pop.
 *
 * Notes:
 *	theta ranges from [-pi, pi]
 *	phi ranges from [0, pi]
 */

#include "mon.h"

#include <err.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "cdefs.h"
#include "cam.h"
#include "env.h"
#include "mach.h"
#include "node.h"
#include "selnode.h"
#include "state.h"
#include "xmath.h"

int revolve_type = REVT_LKAVG;

/*
 *	y
 *	| Up/Down /\
 *	|	  \/
 *	|
 *	| Left/Right < >
 *	+--------------- x
 *     /	      /|
 *    / Back/Forward |/
 *   z
 */
void
cam_move(int dir, double amt)
{
	struct fvec fv, uv;

	if (dir == DIR_LEFT || dir == DIR_BACK || dir == DIR_DOWN)
		amt *= -1;

	switch (dir) {
	case DIR_LEFT:
	case DIR_RIGHT:
		cam_calcuv(&uv);
		vec_crossprod(&fv, &st.st_lv, &uv);
		st.st_v.fv_x += fv.fv_x * amt;
		st.st_v.fv_y += fv.fv_y * amt;
		st.st_v.fv_z += fv.fv_z * amt;
		break;
	case DIR_FORW:
	case DIR_BACK:
		st.st_v.fv_x += st.st_lv.fv_x * amt;
		st.st_v.fv_y += st.st_lv.fv_y * amt;
		st.st_v.fv_z += st.st_lv.fv_z * amt;
		break;
	case DIR_UP:
	case DIR_DOWN:
		cam_calcuv(&uv);
		st.st_v.fv_x += uv.fv_x * amt;
		st.st_v.fv_y += uv.fv_y * amt;
		st.st_v.fv_z += uv.fv_z * amt;
		break;
	}
}

void
cam_revolve(struct fvec *focuspts, int nfocus, double dt, double dp, int revt)
{
	struct fvec minv, maxv, center, diff, sph, sphfocus;
	double maxdiff, dst;
	int j;

	vec_set(&maxv, -DBL_MAX, -DBL_MAX, -DBL_MAX);
	vec_set(&minv, DBL_MAX, DBL_MAX, DBL_MAX);

	vec_set(&center, 0.0, 0.0, 0.0);
	for (j = 0; j < nfocus; j++) {
		center.fv_x += focuspts[j].fv_x;
		center.fv_y += focuspts[j].fv_y;
		center.fv_z += focuspts[j].fv_z;

		minv.fv_x = MIN(minv.fv_x, focuspts[j].fv_x);
		maxv.fv_x = MAX(maxv.fv_x, focuspts[j].fv_x);
		minv.fv_y = MIN(minv.fv_y, focuspts[j].fv_y);
		maxv.fv_y = MAX(maxv.fv_y, focuspts[j].fv_y);
		minv.fv_z = MIN(minv.fv_z, focuspts[j].fv_z);
		maxv.fv_z = MAX(maxv.fv_z, focuspts[j].fv_z);
	}
	center.fv_x /= nfocus;
	center.fv_y /= nfocus;
	center.fv_z /= nfocus;

	/*
	 * Manually change the revolution type to avoid jumps around
	 * extremes and move the camera slightly so we don't
	 * continually toggle this manual switch.
	 */
	if (revt == REVT_LKAVG || revt == REVT_LKFIT) {
		dst = -1.0 + DST(&st.st_v, &center);
		maxdiff = MAX3(
		    maxv.fv_x - minv.fv_x,
		    maxv.fv_y - minv.fv_y,
		    maxv.fv_z - minv.fv_z) / 2.0;
		if (dst < maxdiff) {
			if (maxdiff - dst < 0.1)
				cam_move(DIR_FORW, 0.05);
			revt = REVT_LKCEN;
		}
	}

	vec_sub(&diff, &st.st_v, &center);
	vec_cart2sphere(&diff, &sph);

	if (st.st_urev) {
		dp *= -1.0;
		dt *= -1.0;
	}

	sph.fv_t += dt;
	sph.fv_p += dp;

	if (sph.fv_p < 0.0 || sph.fv_p > M_PI) {
		/*
		 * We flipped upside (or back rightside up),
		 * so move theta onto the other side but
		 * use the same absolute value of phi.
		 */
		sph.fv_t += M_PI;
		sph.fv_p *= -1.0;
		st.st_urev = !st.st_urev;
	}

	sph.fv_t = negmodf(sph.fv_t, 2.0 * M_PI);
//	sph.fv_p = negmodf(sph.fv_p, M_PI);

	vec_sphere2cart(&sph, &st.st_v);
	vec_addto(&center, &st.st_v);

	st.st_lv.fv_x = 0.0;
	st.st_lv.fv_y = 0.0;
	st.st_lv.fv_z = 0.0;
	switch (revt) {
	case REVT_LKAVG:
		for (j = 0; j < nfocus; j++) {
			struct fvec lv;

			lv.fv_x = focuspts[j].fv_x - st.st_v.fv_x;
			lv.fv_y = focuspts[j].fv_y - st.st_v.fv_y;
			lv.fv_z = focuspts[j].fv_z - st.st_v.fv_z;
			vec_normalize(&lv);

			vec_addto(&lv, &st.st_lv);
		}
		st.st_lv.fv_x /= nfocus;
		st.st_lv.fv_y /= nfocus;
		st.st_lv.fv_z /= nfocus;
		break;
	case REVT_LKCEN:
		st.st_lv.fv_x = center.fv_x - st.st_v.fv_x;
		st.st_lv.fv_y = center.fv_y - st.st_v.fv_y;
		st.st_lv.fv_z = center.fv_z - st.st_v.fv_z;
		break;
	case REVT_LKFIT: {
		double mint, minp, maxt, maxp;
		struct fvec lv;

		mint = minp = DBL_MAX;
		maxt = maxp = -DBL_MAX;

		lv.fv_x = center.fv_x - st.st_v.fv_x;
		lv.fv_y = center.fv_y - st.st_v.fv_y;
		lv.fv_z = center.fv_z - st.st_v.fv_z;
		vec_normalize(&lv);
		vec_cart2sphere(&lv, &sphfocus);

		for (j = 0; j < nfocus; j++) {
			lv.fv_x = focuspts[j].fv_x - st.st_v.fv_x;
			lv.fv_y = focuspts[j].fv_y - st.st_v.fv_y;
			lv.fv_z = focuspts[j].fv_z - st.st_v.fv_z;
			vec_normalize(&lv);

			vec_cart2sphere(&lv, &sph);
			if (sph.fv_t - sphfocus.fv_t > M_PI)
				sph.fv_t -= 2.0 * M_PI;
			else if (sph.fv_t - sphfocus.fv_t < -M_PI)
				sph.fv_t += 2.0 * M_PI;
			mint = MIN(mint, sph.fv_t);
			maxt = MAX(maxt, sph.fv_t);
			minp = MIN(minp, sph.fv_p);
			maxp = MAX(maxp, sph.fv_p);
		}
//		sph.fv_r = 1.0;
		sph.fv_t = (mint + maxt) / 2.0;
		sph.fv_p = (minp + maxp) / 2.0;
		vec_sphere2cart(&sph, &st.st_lv);
		break;
	    }
	}
	vec_normalize(&st.st_lv);
}

/*
 * Special case of above: utilizes focus elliptical skews
 * and deals with VM_WIRED mode, which has no focus point
 * or cluster center, since clusters are drawn repeatedly
 * (using revolvefocus should be the standard revolve case).
 */
__inline void
cam_revolvefocus(double dt, double dp)
{
	struct fvec nfv[8], fv, dim, *fvp, *ndim, *nfvp;
	struct selnode *sn;
	double dst;
	int j;

	if (st.st_opts & OP_FORCEFOCUS)
		cam_revolve(&focus, 1, dt, dp, revolve_type);
	else if (nselnodes) {
		if ((fvp = malloc(nselnodes * sizeof(struct fvec))) == NULL)
			err(1, "malloc");
		j = 0;
		SLIST_FOREACH(sn, &selnodes, sn_next) {
			nfvp = &sn->sn_nodep->n_v;
			fvp[j].fv_x = nfvp->fv_x + sn->sn_nodep->n_dimp->fv_w / 2.0;
			fvp[j].fv_y = nfvp->fv_y + sn->sn_nodep->n_dimp->fv_y / 2.0;
			fvp[j].fv_z = nfvp->fv_z + sn->sn_nodep->n_dimp->fv_z / 2.0;

			if (st.st_vmode == VM_WIRED) {
				fvp[j].fv_x += sn->sn_offv.fv_x;
				fvp[j].fv_y += sn->sn_offv.fv_y;
				fvp[j].fv_z += sn->sn_offv.fv_z;
			}

			j++;
		}
		cam_revolve(fvp, j, dt, dp, revolve_type);
		free(fvp);
	} else {
		switch (st.st_vmode) {
		case VM_WIRED:
			dst = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH) / 4.0;
			fv.fv_x = st.st_v.fv_x + st.st_lv.fv_x * dst;
			fv.fv_y = st.st_v.fv_y + st.st_lv.fv_y * dst;
			fv.fv_z = st.st_v.fv_z + st.st_lv.fv_z * dst;
			fvp = &fv;
			cam_revolve(fvp, 1, dt, dp, revolve_type);
			break;
		case VM_WIONE:
			ndim = &vmodes[st.st_vmode].vm_ndim[GEOM_CUBE];
			fv.fv_x = st.st_wioff.iv_x * st.st_winsp.iv_x;
			fv.fv_y = st.st_wioff.iv_y * st.st_winsp.iv_y;
			fv.fv_z = st.st_wioff.iv_z * st.st_winsp.iv_z;
			dim.fv_x = (widim.iv_w - 1) * st.st_winsp.iv_x + ndim->fv_w;
			dim.fv_y = (widim.iv_h - 1) * st.st_winsp.iv_y + ndim->fv_h;
			dim.fv_z = (widim.iv_d - 1) * st.st_winsp.iv_z + ndim->fv_d;

			vec_set(&nfv[0], fv.fv_x,		fv.fv_y,		fv.fv_z);
			vec_set(&nfv[1], fv.fv_x + dim.fv_w,	fv.fv_y,		fv.fv_z);
			vec_set(&nfv[2], fv.fv_x,		fv.fv_y + dim.fv_h,	fv.fv_z);
			vec_set(&nfv[3], fv.fv_x,		fv.fv_y,		fv.fv_z + dim.fv_d);
			vec_set(&nfv[4], fv.fv_x + dim.fv_w,	fv.fv_y + dim.fv_h,	fv.fv_z);
			vec_set(&nfv[5], fv.fv_x + dim.fv_w,	fv.fv_y,		fv.fv_z + dim.fv_d);
			vec_set(&nfv[6], fv.fv_x,		fv.fv_y + dim.fv_h,	fv.fv_z + dim.fv_d);
			vec_set(&nfv[7], fv.fv_x + dim.fv_w,	fv.fv_y + dim.fv_h,	fv.fv_z + dim.fv_d);
			cam_revolve(nfv, NENTRIES(nfv), dt, dp, revolve_type);
			break;
		case VM_PHYS:
			fvp = &machine.m_dim;
			/* XXX not (0,0,0) use machine.m_origin */
			vec_set(&nfv[0], 0.0,		0.0,		0.0);
			vec_set(&nfv[1], fvp->fv_w,	0.0,		0.0);
			vec_set(&nfv[2], 0.0,		fvp->fv_h,	0.0);
			vec_set(&nfv[3], 0.0,		0.0,		fvp->fv_d);
			vec_set(&nfv[4], fvp->fv_w,	fvp->fv_h,	0.0);
			vec_set(&nfv[5], fvp->fv_w,	0.0,		fvp->fv_d);
			vec_set(&nfv[6], 0.0,		fvp->fv_h,	fvp->fv_d);
			vec_set(&nfv[7], fvp->fv_w,	fvp->fv_h,	fvp->fv_d);
			cam_revolve(nfv, NENTRIES(nfv), dt, dp, revolve_type);
			break;
		case VM_VNEIGHBOR:
			cam_revolve(&focus, 1, dt, dp, revolve_type);
			break;
		}
	}
}

__inline void
cam_roll(double amt)
{
	st.st_ur = negmodf(st.st_ur + amt, 2.0 * M_PI);
}

void
cam_bird(int vm)
{
	struct fvec cen, sph;

	switch (vm) {
	case VM_PHYS:
		/* XXX: depend on CL_WIDTH/HEIGHT/DEPTH */
		vec_set(&st.st_v, -17.80, 30.76, 51.92);
		vec_set(&st.st_lv,  0.71, -0.34, -0.62);
		st.st_ur = 0.0;
		st.st_urev = 0.0;
		break;
	case VM_WIRED:
	case VM_WIONE:
		st.st_ur = 0.0;
		st.st_urev = 0.0;
		vec_set(&cen,
		    1.0 * widim.iv_x * st.st_winsp.iv_x,
		    1.0 * widim.iv_y * st.st_winsp.iv_y,
		    1.0 * widim.iv_z * st.st_winsp.iv_z);
		sph.fv_r = 2.7 * widim.iv_x * st.st_winsp.iv_x;
		sph.fv_t = M_PI / 2.0 + atan2(cen.fv_z, cen.fv_x);
		sph.fv_p = M_PI / 3.0;
		vec_sphere2cart(&sph, &st.st_v);
		st.st_v.fv_x += st.st_wioff.iv_x * st.st_winsp.iv_x + cen.fv_w / 2.0;
		st.st_v.fv_y += st.st_wioff.iv_y * st.st_winsp.iv_y + cen.fv_h / 4.0;
		st.st_v.fv_z += st.st_wioff.iv_z * st.st_winsp.iv_z + cen.fv_d / 2.0;
		cam_revolvefocus(0.0, 0.01);
		break;
	case VM_VNEIGHBOR:
		sph.fv_r = 8.0 * ((widim.iv_w + widim.iv_h + widim.iv_d) / 2 + 8);
		sph.fv_t = 5.5 * M_PI * 0.5;
		sph.fv_p = M_PI * 0.25;
		vec_sphere2cart(&sph, &st.st_v);
		vec_set(&st.st_lv, -st.st_v.fv_x, -st.st_v.fv_y, -st.st_v.fv_z);
		vec_normalize(&st.st_lv);
		st.st_ur = 0.0;
		st.st_urev = 0.0;
		break;
	}
}

__inline void
cam_look(void)
{
	struct fvec uv;

	cam_calcuv(&uv);
	glLoadIdentity();
	gluLookAt(st.st_v.fv_x, st.st_v.fv_y, st.st_v.fv_z,
	    st.st_v.fv_x + st.st_lv.fv_x,
	    st.st_v.fv_y + st.st_lv.fv_y,
	    st.st_v.fv_z + st.st_lv.fv_z,
	    uv.fv_x, uv.fv_y, uv.fv_z);
}

void
cam_calcuv(struct fvec *uvp)
{
	struct fvec sph;

	vec_cart2sphere(&st.st_lv, &sph);
	sph.fv_p -= M_PI * 0.5;
	if (st.st_urev)
		sph.fv_p += M_PI;
	vec_sphere2cart(&sph, uvp);
	vec_rotate(uvp, &st.st_lv, st.st_ur);
}
