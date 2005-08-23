/* $Id$ */

#include "compat.h"

#include <math.h>

#include "mon.h"

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
vec_crossprod(struct fvec *cross, struct fvec *a, struct fvec *b)
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

__inline float
vec_mag(struct fvec *fv)
{
	return (sqrt(SQUARE(fv->fv_x) + SQUARE(fv->fv_y) + SQUARE(fv->fv_z)));
}

__inline void
vec_add(struct fvec *a, struct fvec *b)
{
	a->fv_x += b->fv_x;
	a->fv_y += b->fv_y;
	a->fv_z += b->fv_z;
}

#define x cart->fv_x
#define y cart->fv_y
#define z cart->fv_z

#define r sphere->fv_r
#define theta sphere->fv_t
#define phi sphere->fv_p

void
vec_cart2sphere(struct fvec *cart, struct fvec *sphere)
{
	r = sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
	theta = atanf(y / x);
	phi = acosf(z / r);
}

void
vec_sphere2cart(struct fvec *sphere, struct fvec *cart)
{
	x = r * cosf(theta) * sinf(phi);
	y = r * sinf(theta) * sinf(phi);
	z = r * cosf(phi);
}
