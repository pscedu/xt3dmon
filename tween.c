/* $Id$ */

/*
 * This file contains routines related to "tweening"
 * movement, or the code that calculates small intervals
 * of change for smoother animation between one large
 * jump.
 *
 * Tweening is applied to camera position, look, and up
 * orientation vectors.  Tweening is broken into two
 * concepts:
 *
 *	(o) interval -- or the amount of movement of
 *	    between each frame, and
 *	(o) thresold -- when to stop tweening since
 *	    the object is close enough to its destination
 *	    and should finally take on the absolute position.
 */

#include "mon.h"

#include "cdefs.h"
#include "state.h"
#include "tween.h"
#include "xmath.h"

#define TWEEN_MAX_POS		(2.00f)
#define TWEEN_MAX_LOOK		(0.06f)
#define TWEEN_MAX_UP		(0.10f)

double tween_intv = 0.05;
double tween_thres = 0.001;

__inline void
tween_probe(float *cur, float stop, float max, float *scale, float *want)
{
	if (stop - *cur > tween_thres ||
	    stop - *cur < -tween_thres) {
		*want = (stop - *cur) * tween_intv;
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
tween_recalc(float *cur, float scale, float want)
{
	if (want) {
		if (scale < 0.001)
			scale = 0.001;
		*cur += want * scale;
	}
}

__inline void
tween_update(void)
{
	/* XXX: benchmark to test static. */
	static struct fvec want, want_l, want_u;
	static struct fvec sc, sc_l, sc_u;
	static float scale, scale_l, scale_u;

#if 0
	/*
	 * If we have already surpassed more frames
	 * this second than last second, try more
	 * fine-grained movement by decreasing the
	 * tweening interval and increasing the
	 * tween threshold.
	 */
	if (fps_cnt > fps) {
		tween_intv *= fps_cnt / fps;
		tween_thres *= fps / fps_cnt;
	}

#endif

	want.fv_w = want.fv_h = want.fv_d = 0.0f;
	want_l.fv_w = want_l.fv_h = want_l.fv_d = 0.0f;
	want_u.fv_w = want_u.fv_h = want_u.fv_d = 0.0f;

	sc.fv_x = sc.fv_y = sc.fv_z = 1.0f;
	sc_l.fv_x = sc_l.fv_y = sc_l.fv_z = 1.0f;
	sc_u.fv_x = sc_u.fv_y = sc_u.fv_z = 1.0f;

	tween_probe(&st.st_v.fv_x, tv.fv_x, TWEEN_MAX_POS, &sc.fv_x, &want.fv_w);
	tween_probe(&st.st_v.fv_y, tv.fv_y, TWEEN_MAX_POS, &sc.fv_y, &want.fv_h);
	tween_probe(&st.st_v.fv_z, tv.fv_z, TWEEN_MAX_POS, &sc.fv_z, &want.fv_d);

	tween_probe(&st.st_lv.fv_x, tlv.fv_x, TWEEN_MAX_LOOK, &sc_l.fv_x, &want_l.fv_w);
	tween_probe(&st.st_lv.fv_y, tlv.fv_y, TWEEN_MAX_LOOK, &sc_l.fv_y, &want_l.fv_h);
	tween_probe(&st.st_lv.fv_z, tlv.fv_z, TWEEN_MAX_LOOK, &sc_l.fv_z, &want_l.fv_d);

	tween_probe(&st.st_uv.fv_x, tuv.fv_x, TWEEN_MAX_UP, &sc_u.fv_x, &want_u.fv_w);
	tween_probe(&st.st_uv.fv_y, tuv.fv_y, TWEEN_MAX_UP, &sc_u.fv_y, &want_u.fv_h);
	tween_probe(&st.st_uv.fv_z, tuv.fv_z, TWEEN_MAX_UP, &sc_u.fv_z, &want_u.fv_d);

	scale = MIN3(sc.fv_x, sc.fv_y, sc.fv_z);
	scale_l = MIN3(sc_l.fv_x, sc_l.fv_y, sc_l.fv_z);
	scale_u = MIN3(sc_u.fv_x, sc_u.fv_y, sc_u.fv_z);

	tween_recalc(&st.st_v.fv_x, scale, want.fv_w);
	tween_recalc(&st.st_v.fv_y, scale, want.fv_h);
	tween_recalc(&st.st_v.fv_z, scale, want.fv_d);

	tween_recalc(&st.st_lv.fv_x, scale_l, want_l.fv_w);
	tween_recalc(&st.st_lv.fv_y, scale_l, want_l.fv_h);
	tween_recalc(&st.st_lv.fv_z, scale_l, want_l.fv_d);

	tween_recalc(&st.st_uv.fv_x, scale_u, want_u.fv_w);
	tween_recalc(&st.st_uv.fv_y, scale_u, want_u.fv_h);
	tween_recalc(&st.st_uv.fv_z, scale_u, want_u.fv_d);

	if (want.fv_w || want.fv_h || want.fv_d ||
	    want_l.fv_w || want_l.fv_h || want_l.fv_d ||
	    want_u.fv_w || want_u.fv_h || want_u.fv_d)
		st.st_rf |= RF_CAM;
	if (want_l.fv_w || want_l.fv_h || want_l.fv_d)
		vec_normalize(&st.st_lv);
	if (want_u.fv_w || want_u.fv_h || want_u.fv_d)
		vec_normalize(&st.st_uv);
}

static struct fvec sv, slv, suv;

void
tween_push(int opts)
{
	if (st.st_opts & OP_TWEEN) {
		if (opts & TWF_POS) {
			sv.fv_x = st.st_v.fv_x;  st.st_v.fv_x = tv.fv_x;
			sv.fv_y = st.st_v.fv_y;  st.st_v.fv_y = tv.fv_y;
			sv.fv_z = st.st_v.fv_z;  st.st_v.fv_z = tv.fv_z;
		}
		if (opts & TWF_LOOK) {
			slv.fv_x = st.st_lv.fv_x;  st.st_lv.fv_x = tlv.fv_x;
			slv.fv_y = st.st_lv.fv_y;  st.st_lv.fv_y = tlv.fv_y;
			slv.fv_z = st.st_lv.fv_z;  st.st_lv.fv_z = tlv.fv_z;
		}
		if (opts & TWF_UP) {
			suv.fv_x = st.st_uv.fv_x;  st.st_uv.fv_x = tuv.fv_x;
			suv.fv_y = st.st_uv.fv_y;  st.st_uv.fv_y = tuv.fv_y;
			suv.fv_z = st.st_uv.fv_z;  st.st_uv.fv_z = tuv.fv_z;
		}
	}
}

void
tween_pop(int opts)
{
	if (st.st_opts & OP_TWEEN) {
		if (opts & TWF_POS) {
			tv.fv_x = st.st_x;  st.st_v.fv_x = sv.fv_x;
			tv.fv_y = st.st_y;  st.st_v.fv_y = sv.fv_y;
			tv.fv_z = st.st_z;  st.st_v.fv_z = sv.fv_z;
		}
		if (opts & TWF_LOOK) {
			tlv.fv_x = st.st_lv.fv_x;  st.st_lv.fv_x = slv.fv_x;
			tlv.fv_y = st.st_lv.fv_y;  st.st_lv.fv_y = slv.fv_y;
			tlv.fv_z = st.st_lv.fv_z;  st.st_lv.fv_z = slv.fv_z;
		}
		if (opts & TWF_UP) {
			tuv.fv_x = st.st_uv.fv_x;  st.st_uv.fv_x = suv.fv_x;
			tuv.fv_y = st.st_uv.fv_y;  st.st_uv.fv_y = suv.fv_y;
			tuv.fv_z = st.st_uv.fv_z;  st.st_uv.fv_z = suv.fv_z;
		}
	} else
		st.st_rf |= RF_CAM;
}
