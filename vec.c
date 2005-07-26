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
