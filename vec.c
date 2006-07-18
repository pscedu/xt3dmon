/* $Id$ */

#include "mon.h"

#include <math.h>

#include "xmath.h"

int
ivec_eq(const struct ivec *a, const struct ivec *b)
{
	return (
	    a->iv_x == b->iv_x &&
	    a->iv_y == b->iv_y &&
	    a->iv_z == b->iv_z);
}

void
vec_normalize(struct fvec *fvp)
{
	float mag;

	mag = sqrt(SQUARE(fvp->fv_x) + SQUARE(fvp->fv_y) +
	    SQUARE(fvp->fv_z));
	if (mag == 0.0)
		return;
	fvp->fv_x /= mag;
	fvp->fv_y /= mag;
	fvp->fv_z /= mag;
}

void
vec_crossprod(struct fvec *cross, const struct fvec *a, const struct fvec *b)
{
	struct fvec va = *a, vb = *b;

	cross->fv_x = va.fv_y * vb.fv_z - vb.fv_y * va.fv_z;
	cross->fv_y = vb.fv_x * va.fv_z - va.fv_x * vb.fv_z;
	cross->fv_z = va.fv_x * vb.fv_y - vb.fv_x * va.fv_y;
}

__inline void
vec_set(struct fvec *fv, float x, float y, float z)
{
	fv->fv_x = x;
	fv->fv_y = y;
	fv->fv_z = z;
}

__inline void
ivec_set(struct ivec *iv, int x, int y, int z)
{
	iv->iv_x = x;
	iv->iv_y = y;
	iv->iv_z = z;
}

__inline void
vec_copyto(const struct fvec *fva, struct fvec *fvb)
{
	fvb->fv_x = fva->fv_x;
	fvb->fv_y = fva->fv_y;
	fvb->fv_z = fva->fv_z;
}

__inline void
vec_sub(struct fvec *fv, const struct fvec *fva, const struct fvec *fvb)
{
	fv->fv_x = fva->fv_x - fvb->fv_x;
	fv->fv_y = fva->fv_y - fvb->fv_y;
	fv->fv_z = fva->fv_z - fvb->fv_z;
}

__inline float
vec_mag(const struct fvec *fv)
{
	return (sqrt(SQUARE(fv->fv_x) + SQUARE(fv->fv_y) + SQUARE(fv->fv_z)));
}

__inline void
vec_addto(const struct fvec *a, struct fvec *b)
{
	b->fv_x += a->fv_x;
	b->fv_y += a->fv_y;
	b->fv_z += a->fv_z;
}

__inline void
vec_subfrom(const struct fvec *a, struct fvec *b)
{
	b->fv_x -= a->fv_x;
	b->fv_y -= a->fv_y;
	b->fv_z -= a->fv_z;
}

void
vec_rotate(struct fvec *fvp, const struct fvec *axis, double deg)
{
	double m[3][3], rcos, rsin;
	struct fvec fv;

	/*
	 *    rcos + uu(1-rcos)	   w*rsin + vu(1-rcos)	 -v*rsin + wu(1-rcos)
	 * -w*rsin + uv(1-rcos)	     rcos + vv(1-rcos)	  u*rsin + vw(1-rcos)
	 *  v*rsin + uw(1-rcos)	  -u*rsin - vw(1-rcos)	    rcos + ww(1-rcos)
	 */

#define u axis->fv_x
#define v axis->fv_y
#define w axis->fv_z
	rcos = cos(deg);
	rsin = sin(deg);
	m[0][0] =      rcos + u * u * (1 - rcos);
	m[0][1] =  w * rsin + v * u * (1 - rcos);
	m[0][2] = -v * rsin + w * u * (1 - rcos);
	m[1][0] = -w * rsin + u * v * (1 - rcos);
	m[1][1] =      rcos + v * v * (1 - rcos);
	m[1][2] =  u * rsin + w * v * (1 - rcos);
	m[2][0] =  v * rsin + u * w * (1 - rcos);
	m[2][1] = -u * rsin + v * w * (1 - rcos);
	m[2][2] =      rcos + w * w * (1 - rcos);
#undef u
#undef v
#undef w

	fv.fv_x = m[0][0] * fvp->fv_x + m[0][1] * fvp->fv_y + m[0][2] * fvp->fv_z;
	fv.fv_y = m[1][0] * fvp->fv_x + m[1][1] * fvp->fv_y + m[1][2] * fvp->fv_z;
	fv.fv_z = m[2][0] * fvp->fv_x + m[2][1] * fvp->fv_y + m[2][2] * fvp->fv_z;
	*fvp = fv;
	vec_normalize(fvp);
}

#define x cart->fv_x
#define y cart->fv_z	/* These are swapped because */
#define z cart->fv_y	/* OpenGL uses z for depth. */

#define r sphere->fv_r
#define t sphere->fv_t
#define p sphere->fv_p

void
vec_cart2sphere(const struct fvec *cart, struct fvec *sphere)
{
	r = sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));

	if (x == 0.0)
		t = M_PI / 2.0 * SIGNF(y);
	else
		t = atan2(y, x);

	if (r == 0.0)
		p = 0.0;
	else
		p = acos(z / r);
}

void
vec_sphere2cart(const struct fvec *sphere, struct fvec *cart)
{
	x = r * cos(t) * sin(p);
	y = r * sin(t) * sin(p);
	z = r * cos(p);

	if (fabs(x) < 1e-5)
		x = 0.0;
	if (fabs(y) < 1e-5)
		y = 0.0;
	if (fabs(z) < 1e-5)
		z = 0.0;
}
