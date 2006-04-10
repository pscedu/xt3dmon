/* $Id$ */

#include "mon.h"

#include <math.h>

#include "env.h"
#include "state.h"
#include "xmath.h"

/*
 *	y
 *	| Up/Down /\
 *	|	  \/
 *	|
 *	| Left/Right < >
 *	+--------------- x
 *     /	     /\
 *    / Back/Forward \/
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
	case DIR_FORWARD:
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
cam_revolve(struct fvec *center, float d_theta, float d_phi)
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

void
cam_roll(double amt)
{
	vec_rotate(&st.st_uv, &st.st_lv, amt);
}

void
cam_rotate(const struct fvec *begfv, const struct fvec *endfv,
    int signu, int signv)
{
	struct fvec sph, beg_sph, end_sph;
	double dt, dp;
	int upinv, flipt, invbeg, invend;

	flipt = upinv = 0;
	if (st.st_uv.fv_y < 0)
		upinv = !upinv;

	vec_cart2sphere(begfv, &beg_sph);
	vec_cart2sphere(endfv, &end_sph);
	vec_cart2sphere(&st.st_lv, &sph);

#if 0
	if (end_sph.fv_t > M_PI) {
		if (end_sph.fv_p < M_PI / 2.0)
			dp = end_sph.fv_p + beg_sph.fv_p;
		else
			dp = (M_PI - end_sph.fv_p) +
			     (M_PI - beg_sph.fv_p);
	} else
#endif
	/*
	 * inv(erted) | normal
	 *           pi/2
	 *         +--|--+
	 *        /   |   \
	 *       /    |    \
	 *      |     |     | pi
	 *    0 |   theta   |
	 *      |     |     | -pi
	 *       \    |    /
	 *        \   |   /
	 *         +--|--+
	 *          -pi/2
	 */
	invbeg = invend = 0;
	if (beg_sph.fv_t <  M_PI / 2.0 &&
	    beg_sph.fv_t > -M_PI / 2.0)
		invbeg = 1;
	if (end_sph.fv_t <  M_PI / 2.0 &&
	    end_sph.fv_t > -M_PI / 2.0)
		invend = 1;
	if (invbeg && !invend && signv < 0)
//		printf("beg %g,%g,%d\n", end_sph.fv_p, beg_sph.fv_p, signv);
		beg_sph.fv_p += M_PI;
	else if (!invbeg && invend && signv > 0)
	//	printf("end %g,%g,%d\n", end_sph.fv_p, beg_sph.fv_p, signv);
		end_sph.fv_p += M_PI;
	dp = end_sph.fv_p - beg_sph.fv_p;

	if (end_sph.fv_t < beg_sph.fv_t && signu > 0)
		end_sph.fv_t += 2.0 * M_PI;
	else if (end_sph.fv_t > beg_sph.fv_t && signu < 0)
		beg_sph.fv_t += 2.0 * M_PI;
	dt = end_sph.fv_t - beg_sph.fv_t;
printf("%.05f\t%.05f\n", dt, dp);

	/*
	 * The values/logic are somewhat misleading
	 * since they are negative when one would
	 * not expect them to be: when the cursor
	 * moved up but the end_sph moved down, or
	 * the cursor moved down but end_sph moved
	 * up, we flipped.
	 */
	if (upinv) {
//	|| (dp > 0.0 && signy < 0) ||
//	    (dp < 0.0 && signy > 0)) {
		dt *= -1.0;
		dp *= -1.0;
		/*
		 * We shouldn't need to set upinv
		 * since it will get set for the
		 * (phi < 0) handling below.
		 */
	}

	sph.fv_t += dt;
	sph.fv_p += dp;
// printf("dt: %g, dp: %g (%g,%g) =>%d\n", dt, dp, end_sph.fv_p, beg_sph.fv_p, signy);

if (sph.fv_p > M_PI)
 printf("  @@@@@ phi: %g > M_PI\n", sph.fv_p);
//	if (sph.fv_p > M_PI)
//		sph.fv_p = M_PI - sph.fv_p;
	if (sph.fv_p < 0.0) {
		sph.fv_t += M_PI;
		sph.fv_p *= -1.0;
		upinv = !upinv;
flipt=1;
	}

	sph.fv_t = negmodf(sph.fv_t, 2.0 * M_PI);

	vec_sphere2cart(&sph, &st.st_lv);
	vec_normalize(&st.st_lv);

#if 0
if (flipt) {
	sph.fv_t -= M_PI;
	sph.fv_p *= -1.0;
}
#endif

	sph.fv_p += M_PI / 2.0;
	if (upinv)
		sph.fv_p += M_PI;
	vec_sphere2cart(&sph, &st.st_uv);
	st.st_uv.fv_x *= -1.0;
	st.st_uv.fv_y *= -1.0;
	st.st_uv.fv_z *= -1.0;
	vec_normalize(&st.st_uv);
}

void
cam_getspecvec(struct fvec *fvp, int u, int v)
{
	struct fvec sph;

	vec_cart2sphere(&st.st_lv, &sph);
	/* XXX: st.st_uv.fv_y < 0 */
	sph.fv_t += DEG_TO_RAD(FOVY) * ASPECT * (u - winv.iv_w / 2.0) / winv.iv_w;
	sph.fv_p += DEG_TO_RAD(FOVY) *          (v - winv.iv_h / 2.0) / winv.iv_h;
	vec_sphere2cart(&sph, fvp);
	vec_normalize(fvp);
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
