/* $Id$ */

/*
 * Camera routines.
 * These functions all deal directly with the current
 * camera position, so make sure you surround any calls
 * to them with tween_push/pop.
 */

#include "mon.h"

#include <math.h>

#include "cdefs.h"
#include "cam.h"
#include "env.h"
#include "selnode.h"
#include "state.h"
#include "xmath.h"

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
	static struct fvec fv;

	if (dir == DIR_LEFT ||
	    dir == DIR_BACK ||
	    dir == DIR_DOWN)
		amt *= -1;

	switch (dir) {
	case DIR_LEFT:
	case DIR_RIGHT:
		vec_crossprod(&fv, &st.st_lv, &st.st_uv);
		st.st_x += fv.fv_x * amt;
		st.st_y += fv.fv_y * amt;
		st.st_z += fv.fv_z * amt;
		break;
	case DIR_FORW:
	case DIR_BACK:
		st.st_x += st.st_lx * amt;
		st.st_y += st.st_ly * amt;
		st.st_z += st.st_lz * amt;
		break;
	case DIR_UP:
	case DIR_DOWN:
		st.st_x += st.st_uv.fv_x * amt;
		st.st_y += st.st_uv.fv_y * amt;
		st.st_z += st.st_uv.fv_z * amt;
		break;
	}
}

void
cam_revolve(struct fvec *center, int nfocus, double dt, double dp, int revt)
{
	struct fvec focuspt, diff, sph;
	int upinv, j;

	upinv = 0;
	if (st.st_uv.fv_y < 0.0) {
		/*
		 * If we are already looking upside,
		 * direction is reversed.
		 */
		upinv = !upinv;
		dp *= -1.0;
		dt *= -1.0;
	}

	focuspt.fv_x = 0.0;
	focuspt.fv_y = 0.0;
	focuspt.fv_z = 0.0;
	for (j = 0; j < nfocus; j++) {
		focuspt.fv_x += center[j].fv_x;
		focuspt.fv_y += center[j].fv_y;
		focuspt.fv_z += center[j].fv_z;
	}
	focuspt.fv_x /= nfocus;
	focuspt.fv_y /= nfocus;
	focuspt.fv_z /= nfocus;

	vec_sub(&diff, &st.st_v, &focuspt);
	vec_cart2sphere(&diff, &sph);

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
		upinv = !upinv;
	}

	sph.fv_t = negmodf(sph.fv_t, 2.0 * M_PI);
//	sph.fv_p = negmodf(sph.fv_p, M_PI);

	vec_sphere2cart(&sph, &st.st_v);
	vec_addto(&focuspt, &st.st_v);

	st.st_lv.fv_x = 0.0;
	st.st_lv.fv_y = 0.0;
	st.st_lv.fv_z = 0.0;
	switch (revt) {
	case REVT_LKAVG:
		for (j = 0; j < nfocus; j++) {
			struct fvec lv;

			lv.fv_x = center[j].fv_x - st.st_v.fv_x;
			lv.fv_y = center[j].fv_y - st.st_v.fv_y;
			lv.fv_z = center[j].fv_z - st.st_v.fv_z;
			vec_normalize(&lv);

			vec_addto(&lv, &st.st_lv);
		}
		st.st_lv.fv_x /= nfocus;
		st.st_lv.fv_y /= nfocus;
		st.st_lv.fv_z /= nfocus;
		break;
	case REVT_LKCEN:
		st.st_lv.fv_x = focuspt.fv_x - st.st_v.fv_x;
		st.st_lv.fv_y = focuspt.fv_y - st.st_v.fv_y;
		st.st_lv.fv_z = focuspt.fv_z - st.st_v.fv_z;
		break;
	}
	vec_normalize(&st.st_lv);

	vec_cart2sphere(&st.st_lv, &sph);
	sph.fv_p -= M_PI / 2.0;
	if (upinv)
		sph.fv_p += M_PI;
	vec_sphere2cart(&sph, &st.st_uv);
	vec_normalize(&st.st_uv);
}

/*
 * Special case of above: utilizes focus elliptical skews
 * and deals with VM_WIRED mode, which has no focus point
 * or cluster center, since clusters are drawn repeatedly
 * (using revolvefocus should be the standard revolve case).
 */
__inline void
cam_revolvefocus(double dt, double dp, int type)
{
	struct fvec *fvp;
	struct fvec fv;
	double dst;

	fvp = &focus;
	if (st.st_vmode == VM_WIRED && nselnodes == 0) {
		dst = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH) / 4.0;
		fv.fv_x = st.st_x + st.st_lx * dst;
		fv.fv_y = st.st_y + st.st_ly * dst;
		fv.fv_z = st.st_z + st.st_lz * dst;
		fvp = &fv;
	}

	if (st.st_vmode == VM_PHYS && nselnodes == 0) {
		struct fvec nfv[2] = {
			{ { NODESPACE, NODESPACE + CL_HEIGHT / 2.0, CL_DEPTH / 2.0 } },
			{ { NODESPACE + CL_WIDTH, NODESPACE + CL_HEIGHT / 2.0, CL_DEPTH / 2.0 } }
		};

		if (type == REVT_LKAVG &&
		    DST(&st.st_v, &focus) < CL_WIDTH / 2.0)
			type = REVT_LKCEN;

		cam_revolve(nfv, 2, dt, dp, type);
	} else
		cam_revolve(fvp, 1, dt, dp, type);
}

void
cam_roll(double amt)
{
	vec_rotate(&st.st_uv, &st.st_lv, amt);
}

void
cam_bird(void)
{
	struct fvec cen, sph;

	switch (st.st_vmode) {
	case VM_PHYS:
		/* XXX: move to fit CL_WIDTH/HEIGHT/DEPTH in view? */
		vec_set(&st.st_v, -17.80, 30.76, 51.92);
		vec_set(&st.st_lv,  0.71, -0.34, -0.62);
		vec_set(&st.st_uv,  0.25,  0.94, -0.22);
		break;
	case VM_WIRED:
	case VM_WIONE:
		vec_set(&cen,
		    1.0 * widim.iv_x * st.st_winsp.iv_x,
		    1.0 * widim.iv_y * st.st_winsp.iv_y,
		    1.0 * widim.iv_z * st.st_winsp.iv_z);
		sph.fv_r = 2.5 * widim.iv_x * st.st_winsp.iv_x;
		sph.fv_t = M_PI / 2.0 + atan2(cen.fv_z, cen.fv_x);
		sph.fv_p = M_PI / 4.0;
		vec_sphere2cart(&sph, &st.st_v);
		st.st_v.fv_x += st.st_wioff.iv_x * st.st_winsp.iv_x + cen.fv_w / 2.0;
		st.st_v.fv_y += st.st_wioff.iv_y * st.st_winsp.iv_y + cen.fv_h / 2.0;
		st.st_v.fv_z += st.st_wioff.iv_z * st.st_winsp.iv_z + cen.fv_d / 2.0;
		cam_revolvefocus(0.0, 0.01, REVT_LKAVG);
		break;
	}
}

__inline void
cam_look(void)
{
	glLoadIdentity();
	gluLookAt(st.st_x, st.st_y, st.st_z,
	    st.st_x + st.st_lx,
	    st.st_y + st.st_ly,
	    st.st_z + st.st_lz,
	    st.st_uv.fv_x,
	    st.st_uv.fv_y,
	    st.st_uv.fv_z);
}
