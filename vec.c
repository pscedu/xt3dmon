/* $Id$ */

#include "compat.h"

#include <math.h>

#include "mon.h"
#include "xmath.h"

void
vec_normalize(struct fvec *fvp)
{
	float mag;

	mag = sqrt(SQUARE(fvp->fv_x) + SQUARE(fvp->fv_y) +
	    SQUARE(fvp->fv_z));
	fvp->fv_x /= mag;
	fvp->fv_y /= mag;
	fvp->fv_z /= mag;
}

void
vec_crossprod(struct fvec *cross, const struct fvec *a, const struct fvec *b)
{
	cross->fv_x = a->fv_y * b->fv_z - b->fv_y * a->fv_z;
	cross->fv_y = b->fv_x * a->fv_z - a->fv_x * b->fv_z;
	cross->fv_z = a->fv_x * b->fv_y - b->fv_x * a->fv_y;
}

__inline void
vec_set(struct fvec *fv, float x, float y, float z)
{
	fv->fv_x = x;
	fv->fv_y = y;
	fv->fv_z = z;
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

void
vec_rotate(struct fvec *fv, const struct fvec *axis, double deg)
{
	struct fvec cr;
	double m[16];

	

/*
	vec_crossprod(&cr, fv, axis);

	(1-cos t)XX + cos t	(1-cos t)XY + (sin t)Z	(1-cos t)XZ - (sin t)Y	0
	(1-cos t)XY-(sin t)Z	(1-cos t)YY + cos t	(1-cos t)YZ + (sin t)X	0
	(1-cos t)XY + (sin t)Y	(1-cos t)YZ - (sin t)X	(1-cos t)ZZ + cos t	0
	0			0			0			1
*/
}

#define x cart->fv_x
#define y cart->fv_z	/* These are swapped because */
#define z cart->fv_y	/* OpenGL uses y for depth. */

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
