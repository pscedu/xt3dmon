/* $Id$ */

#include "compat.h"
#include "mon.h"

#define TWEEN_THRES	(0.001f)
#define TWEEN_AMT	(0.05f)

int cam_dirty = 1;

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

static struct fvec sv, slv;

void
tween_push(int opts)
{
	if (st.st_opts & OP_TWEEN) {
		if (opts & TWF_POS) {
			sv.fv_x = st.st_x;  st.st_x = tv.fv_x;
			sv.fv_y = st.st_y;  st.st_y = tv.fv_y;
			sv.fv_z = st.st_z;  st.st_z = tv.fv_z;
		}
		if (opts & TWF_LOOK) {
			slv.fv_x = st.st_lx;  st.st_lx = tlv.fv_x;
			slv.fv_y = st.st_ly;  st.st_ly = tlv.fv_y;
			slv.fv_z = st.st_lz;  st.st_lz = tlv.fv_z;
		}
	}
}

void
tween_pop(int opts)
{
	if (st.st_opts & OP_TWEEN) {
		if (opts & TWF_POS) {
			tv.fv_x = st.st_x;  st.st_x = sv.fv_x;
			tv.fv_y = st.st_y;  st.st_y = sv.fv_y;
			tv.fv_z = st.st_z;  st.st_z = sv.fv_z;
		}
		if (opts & TWF_LOOK) {
			tlv.fv_x = st.st_lx;  st.st_lx = slv.fv_x;
			tlv.fv_y = st.st_ly;  st.st_ly = slv.fv_y;
			tlv.fv_z = st.st_lz;  st.st_lz = slv.fv_z;
		}
	} else
		cam_dirty = 1;
}
