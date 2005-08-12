/* $Id$ */

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
