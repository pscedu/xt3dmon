/* $Id$ */

#include "mon.h"

#include <float.h>

#include "env.h"
#include "lnseg.h"
#include "mark.h"
#include "state.h"
#include "tween.h"

/*

-sin(x*3.14159/2*3/4)*3/4
(1-1/4x^2)^(1/2)

*/

#define DST(x1, y1, x2, y2) 				\
	sqrt(((x2) - (x1)) * ((x2) - (x1)) +		\
	     ((y2) - (y1)) * ((y2) - (y1)))

void
dx_update(void)
{
}

int
dx_cue(double d)
{
	static double t;
	static int dim = 1;
	double roll, a, b, max;
	struct fvec sv, uv, lv, xv;
	int ret;

	ret = 0;
	if (t > 4 * M_PI) {
		t = 0;
//		dim++;

		if (dim >= NDIM) {
			ret = 1;
			dim = 0;
		}
	}

	switch (dim) {
	case DIM_X:
		a = CABHEIGHT/4.0;
		b = ROWDEPTH/4.0;
		break;
	case DIM_Y:
		a = ROWWIDTH/4.0 - CABWIDTH;
		b = CABHEIGHT/2.0;
		break;
	case DIM_Z:
		a = ROWWIDTH/4.0 - CABWIDTH;
		b = CABHEIGHT/2.0;
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

	switch (dim) {
	case DIM_X:
		st.st_x = XCENTER;
		st.st_y = YCENTER + sv.fv_x;
		st.st_z = ZCENTER + sv.fv_y;

		st.st_ux = 0.0;
		st.st_uy = uv.fv_x;
		st.st_uz = uv.fv_y;

		st.st_lx = 0.0;
		st.st_ly = lv.fv_x;
		st.st_lz = lv.fv_y;
		break;
	case DIM_Y:
		st.st_x = XCENTER + sv.fv_x;
		st.st_y = YCENTER;
		st.st_z = ZCENTER + sv.fv_y;

		st.st_ux = uv.fv_x;
		st.st_uy = 0.0;
		st.st_uz = uv.fv_y;

		st.st_lx = lv.fv_x;
		st.st_ly = 0.0;
		st.st_lz = lv.fv_y;
		break;
	case DIM_Z:
		st.st_x = XCENTER + sv.fv_x;
		st.st_y = YCENTER + sv.fv_y;
		st.st_z = ZCENTER;

		st.st_ux = uv.fv_x;
		st.st_uy = uv.fv_y;
		st.st_uz = 0.0;

		st.st_lx = lv.fv_x;
		st.st_ly = lv.fv_y;
		st.st_lz = 0.0;
		break;
	}

//st.st_lx = -st.st_x;
//st.st_ly = -st.st_y;
//st.st_lz = -st.st_z;
//vec_normalize(&st.st_lv);

	roll = 0.0;
	if (t > M_PI / 2.0 && t < 3 * M_PI / 2.0)
		roll = t - M_PI / 2.0;
	else if (t > 3 * M_PI / 2.0 && t < 5 * M_PI/2.0)
		roll = M_PI;
	else if (t > 5 * M_PI / 2.0 && t < 7 * M_PI / 2.0)
		roll = M_PI + t - 5 * M_PI / 2.0;

	while (roll > 0.1) {
		cam_roll(0.1);
		roll -= 0.1;
	}

	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += 0.05;

	return (ret);
}
