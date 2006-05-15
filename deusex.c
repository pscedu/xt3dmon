/* $Id$ */

#include "mon.h"

#include <float.h>

#include "env.h"
#include "lnseg.h"
#include "mark.h"
#include "state.h"

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
	static double lastdiff = DBL_MAX;
	static double t;
	double a, b, max, diff;
	struct fvec sv, ev;
	int ret;

	a = ROWWIDTH/4.0 - CABWIDTH;
	b = CABHEIGHT/2.0;
	max = MAX(a, b);

	if (t > 4 * M_PI)
		t = 0;

	if (t < 2 * M_PI) {
		sv.fv_x = a * sin(t - M_PI/2) + max;
		sv.fv_y = b * cos(t - M_PI/2);
		sv.fv_z = 0.0;
	} else {
		sv.fv_x = -a * sin(t - M_PI/2) - max;
		sv.fv_y = b * cos(t - M_PI/2);
		sv.fv_z = 0.0;
	}

	ev.fv_x = 2 * sv.fv_x;
	ev.fv_y = 2 * sv.fv_y;
	ev.fv_z = 0.0;
	vec_normalize(&ev);

	sv.fv_x += XCENTER;
	sv.fv_y += YCENTER;
	sv.fv_z += ZCENTER;

	ev.fv_x += sv.fv_x;
	ev.fv_y += sv.fv_y;
	ev.fv_z += sv.fv_z;

	lnseg_add(&sv, &ev);
	mark_add(&sv);

	t += 0.1;

return 0;



//	tween_push(TWF_LOOK | TWF_POS | TWF_UP);

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
