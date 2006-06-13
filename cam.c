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
cam_move(int dir, float amt)
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
cam_revolve(struct fvec *center, double d_theta, double d_phi)
{
	struct fvec diff, sph;
	int upinv;

	upinv = 0;
	if (st.st_uv.fv_y < 0.0) {
		/*
		 * If we are already looking upside,
		 * direction is reversed.
		 */
		upinv = !upinv;
		d_phi *= -1;
		d_theta *= -1;
	}

	vec_sub(&diff, &st.st_v, center);
	vec_cart2sphere(&diff, &sph);
	sph.fv_t += 0.01 * d_theta;
	sph.fv_p += 0.01 * d_phi;
//	sph.fv_p = negmodf(sph.fv_p, 2.0 * M_PI);

	if (sph.fv_p < 0.0) {
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

	/* Point the look vector at the center of revolution. */
	vec_sphere2cart(&sph, &st.st_v);
	st.st_lv.fv_x = -st.st_v.fv_x;
	st.st_lv.fv_y = -st.st_v.fv_y;
	st.st_lv.fv_z = -st.st_v.fv_z;
	vec_normalize(&st.st_lv);

	sph.fv_p += M_PI / 2.0;
	if (upinv)
		sph.fv_p += M_PI;
	vec_sphere2cart(&sph, &st.st_uv);
	st.st_uv.fv_x *= -1.0;
	st.st_uv.fv_y *= -1.0;
	st.st_uv.fv_z *= -1.0;
	vec_normalize(&st.st_uv);

	vec_addto(center, &st.st_v);
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
	struct fvec *fvp;
	struct fvec fv;
	double dst;

	fvp = &focus;
	if (st.st_vmode == VM_WIRED &&
	    nselnodes == 0) {
		dst = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH) / 4.0;
		fv.fv_x = st.st_x + st.st_lx * dst;
		fv.fv_y = st.st_y + st.st_ly * dst;
		fv.fv_z = st.st_z + st.st_lz * dst;
		fvp = &fv;
	}

	cam_revolve(fvp, dt, dp);
}

void
cam_roll(double amt)
{
	vec_rotate(&st.st_uv, &st.st_lv, amt);
}

void
cam_bird(void)
{
	switch (st.st_vmode) {
	case VM_PHYS:
		/* XXX: move to fit CL_WIDTH/HEIGHT/DEPTH in view? */
		vec_set(&st.st_v, -17.80, 30.76, 51.92);
		vec_set(&st.st_lv,  0.71, -0.34, -0.62);
		vec_set(&st.st_uv,  0.25,  0.94, -0.22);
		break;
	case VM_WIRED:
	case VM_WIONE:
		/* XXX: move to fit WI_WIDTH/HEIGHT/DEPTH in view? */
		vec_set(&st.st_v, -45.90, 82.70, 93.60);
		vec_set(&st.st_lv,  0.60, -0.57, -0.55);
		vec_set(&st.st_uv,  0.43,  0.82, -0.38);
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
