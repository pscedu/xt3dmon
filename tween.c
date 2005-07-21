/* $Id$ */

#include "mon.h"

#define TWEEN_THRES	(0.001f)
#define TWEEN_AMT	(0.05f)

__inline void
tween_probe(float *cur, float stop, float max, float *scale, float *want)
{
	if (stop != *cur) {
		*want = (stop - *cur) * TWEEN_AMT;
		if (*want == 0.0f) {
			/* recalc() won't get called. */
			*cur = stop;
		} else if (*want > max) {
			*scale = max / *want;
			if (*scale < 0.0f)
				*scale *= -1.0f;
		}
	}
}

__inline void
tween_recalc(float *cur, float stop, float scale, float want)
{
	if (want) {
		if (scale < 0.001)
			scale = 0.001;
		*cur += want * scale;
		if (stop - *cur < TWEEN_THRES &&
		    stop - *cur > -TWEEN_THRES)
			*cur = stop;
	}
}

static float sx, sy, sz, slx, sly, slz;

void
tween_push(int opts)
{
	if (st.st_opts & OP_TWEEN) {
		if (opts & TWF_POS) {
			ox = st.st_x;  sx = st.st_x;  st.st_x = tx;
			oy = st.st_y;  sy = st.st_y;  st.st_y = ty;
			oz = st.st_z;  sz = st.st_z;  st.st_z = tz;
		}
		if (opts & TWF_LOOK) {
			olx = st.st_lx;  slx = st.st_lx;  st.st_lx = tlx;
			oly = st.st_ly;  sly = st.st_ly;  st.st_ly = tly;
			olz = st.st_lz;  slz = st.st_lz;  st.st_lz = tlz;
		}
	}
}

void
tween_pop(int opts)
{
	if (st.st_opts & OP_TWEEN) {
		if (opts & TWF_POS) {
			tx = st.st_x;  st.st_x = sx;
			ty = st.st_y;  st.st_y = sy;
			tz = st.st_z;  st.st_z = sz;
		}
		if (opts & TWF_LOOK) {
			tlx = st.st_lx;  st.st_lx = slx;
			tly = st.st_ly;  st.st_ly = sly;
			tlz = st.st_lz;  st.st_lz = slz;
		}
	} else {
		cam_update();
	}
}
