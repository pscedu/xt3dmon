/* $Id$ */

#include <float.h>

#include "mark.h"

/*

-sin(x*3.14159/2*3/4)*3/4
(1-1/4x^2)^(1/2)

*/

#define DST(x1, y1, x2, y2) 				\
	sqrt(((x2) - (x1)) * ((x2) - (x1)) +		\
	     ((y2) - (y1)) * ((y2) - (y1)))

int
dx_cue(double d)
{
	static double lastdiff = DBL_MAX;
	static double t;
	double diff;
	struct fvec sv, ev;
	double a = 200;
	double b = 100;
	int ret;

//	tween_push(TWF_LOOK | TWF_POS | TWF_UP);

	sv.fv_x = a * t;
	sv.fv_z = (b * 3 / 4) * sin(t * M_PI * 8 / 9);
	sv.fv_y = 0;

	ev.fv_x = a * sin(t);
	ev.fv_z = b * cos(t);
	ev.fv_y = 0;

	diff = DST(ev.fv_x, ev.fv_z, sv.fv_x, sv.fv_z);

//	if (diff > lastdiff) {
//		st.st_x = ev.fv_x;
//		st.st_z = ev.fv_z;
//	} else {
//		st.st_x = sv.fv_x;
//		st.st_z = sv.fv_z;
//	}
//	lastdiff = diff;

	mark_add(&sv);
	mark_add(&ev);

//	st.st_lx = -st.st_x;
//	st.st_lz = -st.st_z;
//	st.st_ly = 0;

//	vec_normalize(&st.st_lv);
//	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += .01;
	if (t >= 2 * M_PI) {
		t = 0;
		/* go into new dimension (x,y -> x,z) */
		ret = 1;
	}
	return (ret);
}
