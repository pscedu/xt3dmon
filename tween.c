/* $Id$ */

#include "compat.h"
#include "cdefs.h"
#include "mon.h"

#define TWEEN_THRES	(0.001f)
#define TWEEN_AMT	(0.05f)

#define TWEEN_MAX_POS	(2.00f)
#define TWEEN_MAX_LOOK	(0.06f)
#define TWEEN_MAX_UP	(0.10f)

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

__inline void
tween_update(void)
{
	/* XXX: benchmark to test static. */
	static struct fvec want, want_l, want_u;
	static struct fvec sc, sc_l, sc_u;
	static float scale, scale_l, scale_u;

	want.fv_w = want.fv_h = want.fv_d = 0.0f;
	want_l.fv_w = want_l.fv_h = want_l.fv_d = 0.0f;
	want_u.fv_w = want_u.fv_h = want_u.fv_d = 0.0f;

	sc.fv_x = sc.fv_y = sc.fv_z = 1.0f;
	sc_l.fv_x = sc_l.fv_y = sc_l.fv_z = 1.0f;
	sc_u.fv_x = sc_u.fv_y = sc_u.fv_z = 1.0f;

	tween_probe(&st.st_x, tv.fv_x, TWEEN_MAX_POS, &sc.fv_x, &want.fv_w);
	tween_probe(&st.st_y, tv.fv_y, TWEEN_MAX_POS, &sc.fv_y, &want.fv_h);
	tween_probe(&st.st_z, tv.fv_z, TWEEN_MAX_POS, &sc.fv_z, &want.fv_d);

	tween_probe(&st.st_lx, tlv.fv_x, TWEEN_MAX_LOOK, &sc_l.fv_x, &want_l.fv_w);
	tween_probe(&st.st_ly, tlv.fv_y, TWEEN_MAX_LOOK, &sc_l.fv_y, &want_l.fv_h);
	tween_probe(&st.st_lz, tlv.fv_z, TWEEN_MAX_LOOK, &sc_l.fv_z, &want_l.fv_d);

	tween_probe(&st.st_ux, tuv.fv_x, TWEEN_MAX_UP, &sc_u.fv_x, &want_u.fv_w);
	tween_probe(&st.st_uy, tuv.fv_y, TWEEN_MAX_UP, &sc_u.fv_y, &want_u.fv_h);
	tween_probe(&st.st_uz, tuv.fv_z, TWEEN_MAX_UP, &sc_u.fv_z, &want_u.fv_d);

	scale = MIN3(sc.fv_x, sc.fv_y, sc.fv_z);
	scale_l = MIN3(sc_l.fv_x, sc_l.fv_y, sc_l.fv_z);
	scale_u = MIN3(sc_u.fv_x, sc_u.fv_y, sc_u.fv_z);

	tween_recalc(&st.st_x, tv.fv_x, scale, want.fv_w);
	tween_recalc(&st.st_y, tv.fv_y, scale, want.fv_h);
	tween_recalc(&st.st_z, tv.fv_z, scale, want.fv_d);

	tween_recalc(&st.st_lx, tlv.fv_x, scale_l, want_l.fv_w);
	tween_recalc(&st.st_ly, tlv.fv_y, scale_l, want_l.fv_h);
	tween_recalc(&st.st_lz, tlv.fv_z, scale_l, want_l.fv_d);

	tween_recalc(&st.st_ux, tuv.fv_x, scale_u, want_u.fv_w);
	tween_recalc(&st.st_uy, tuv.fv_y, scale_u, want_u.fv_h);
	tween_recalc(&st.st_uz, tuv.fv_z, scale_u, want_u.fv_d);

	if (want.fv_w || want.fv_h || want.fv_d ||
	    want_l.fv_w || want_l.fv_h || want_l.fv_d ||
	    want_u.fv_w || want_u.fv_h || want_u.fv_d)
		cam_dirty = 1;
}

static struct fvec sv, slv, suv;

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
		if (opts & TWF_UP) {
			suv.fv_x = st.st_ux;  st.st_ux = tuv.fv_x;
			suv.fv_y = st.st_uy;  st.st_uy = tuv.fv_y;
			suv.fv_z = st.st_uz;  st.st_uz = tuv.fv_z;
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
		if (opts & TWF_UP) {
			tuv.fv_x = st.st_ux;  st.st_ux = suv.fv_x;
			tuv.fv_y = st.st_uy;  st.st_uy = suv.fv_y;
			tuv.fv_z = st.st_uz;  st.st_uz = suv.fv_z;
		}
	} else
		cam_dirty = 1;
}
