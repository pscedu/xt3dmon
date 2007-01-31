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
 *	(o) scale -- performance degradation factor used
 *	    to compute threshold of when the object is
 *	    close enough to its destination.
 */

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "state.h"
#include "tween.h"
#include "xmath.h"

#define TWEEN_MAXADJ_POS	(2.00f)
#define TWEEN_MAXADJ_LOOK	(0.06f)
#define TWEEN_MAXADJ_UPROT	(0.06f)

#define TWEEN_THRES_POS		(0.005f)
#define TWEEN_THRES_LOOK	(0.001f)
#define TWEEN_THRES_UPROT	(0.0001f)

double	tween_intv = 0.05;	/* 5% */
double	tween_scale = 1.0;	/* 100% (no perf. degradation) */
int	tween_active;		/* If inside tween_push/pop() */

/* Current tween values. */
static struct fvec tv, tlv;
static float tur;
static int turev;

/* Saved tween values. */
static struct fvec sv, slv;
static float sur;
static int surev;

__inline void
tween_probe(float *cur, float stop, float max, float *scale, float *want, float thres)
{
	float diff;

	diff = stop - *cur;
	if (fabs(diff) > thres) {
		*want = diff * tween_intv;
		if (fabs(*want * 100.0) < thres) {
			*cur = stop;
			*want = 0.0;
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
	static float scale, scale_l, scale_ur, want_ur;
	static struct fvec lsph, tlsph;
	static struct fvec want, want_l;
	static struct fvec sc, sc_l;
	float ur;

	if (tween_active)
		errx(1, "tweening is active");

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
		tween_scale *= fps / fps_cnt;
	}

#endif

	want.fv_w = want.fv_h = want.fv_d = 0.0f;
	want_l.fv_t = want_l.fv_p = 0.0f;
	want_ur = 0.0;

	sc.fv_x = sc.fv_y = sc.fv_z = 1.0f;
	sc_l.fv_t = sc_l.fv_p = 1.0f;
	scale_ur = 1.0;

	tween_probe(&st.st_x, tv.fv_x, TWEEN_MAXADJ_POS, &sc.fv_x, &want.fv_w, TWEEN_THRES_POS);
	tween_probe(&st.st_y, tv.fv_y, TWEEN_MAXADJ_POS, &sc.fv_y, &want.fv_h, TWEEN_THRES_POS);
	tween_probe(&st.st_z, tv.fv_z, TWEEN_MAXADJ_POS, &sc.fv_z, &want.fv_d, TWEEN_THRES_POS);

	vec_cart2sphere(&tlv, &tlsph);
	vec_cart2sphere(&st.st_lv, &lsph);
	if (st.st_urev != turev) {
		float phi;

		/* When phi wraps, go to the nearest repeat. */
		if (2.0 * tlsph.fv_p > M_PI)
			phi = M_PI + (M_PI - tlsph.fv_p);
		else
			phi = -tlsph.fv_p;
		tween_probe(&lsph.fv_p, phi, TWEEN_MAXADJ_LOOK, &sc_l.fv_p, &want_l.fv_p, TWEEN_THRES_LOOK);
	} else {
		/* When theta wraps, go to the nearest repeat. */
		if (2.0 * lsph.fv_t < -M_PI && 2.0 * tlsph.fv_t > M_PI)
			lsph.fv_t += 2.0 * M_PI;
		else if (2.0 * lsph.fv_t > M_PI && 2.0 * tlsph.fv_t < -M_PI)
			tlsph.fv_t += 2.0 * M_PI;
		tween_probe(&lsph.fv_t, tlsph.fv_t, TWEEN_MAXADJ_LOOK, &sc_l.fv_t, &want_l.fv_t, TWEEN_THRES_LOOK);
		tween_probe(&lsph.fv_p, tlsph.fv_p, TWEEN_MAXADJ_LOOK, &sc_l.fv_p, &want_l.fv_p, TWEEN_THRES_LOOK);
	}

	if (st.st_ur > 1.5 * M_PI && 2.0 * tur < M_PI)
		/* We went over 2 * PI. */
		ur = tur + 2.0 * M_PI;
	else if (tur > 1.5 * M_PI && 2.0 * st.st_ur < M_PI)
		/* We went under 0. */
		ur = tur - 2.0 * M_PI;
	else
		ur = tur;
	tween_probe(&st.st_ur, ur, TWEEN_MAXADJ_UPROT, &scale_ur, &want_ur, TWEEN_THRES_UPROT);

	scale = MIN3(sc.fv_x, sc.fv_y, sc.fv_z);
	scale_l = MIN(sc_l.fv_t, sc_l.fv_p);

	tween_recalc(&st.st_x, scale, want.fv_w);
	tween_recalc(&st.st_y, scale, want.fv_h);
	tween_recalc(&st.st_z, scale, want.fv_d);

	if (want_l.fv_t || want_l.fv_p) {
		tween_recalc(&lsph.fv_t, scale_l, want_l.fv_t);
		tween_recalc(&lsph.fv_p, scale_l, want_l.fv_p);
		if (lsph.fv_p > M_PI || lsph.fv_p < 0)
			st.st_urev = !st.st_urev;
		lsph.fv_r = 1.0;
		vec_sphere2cart(&lsph, &st.st_lv);
	}

	tween_recalc(&st.st_ur, scale_ur, want_ur);
	st.st_ur = negmodf(st.st_ur, 2 * M_PI);

	if (want.fv_w || want.fv_h || want.fv_d ||
	    want_l.fv_t || want_l.fv_p || want_ur)
		st.st_rf |= RF_CAM;
}

void
tween_push(void)
{
	tween_active = 1;
	if (st.st_opts & OP_TWEEN) {
		sv = st.st_v;
		slv = st.st_lv;
		sur = st.st_ur;
		surev = st.st_urev;

		st.st_v    = tv;
		st.st_lv   = tlv;
		st.st_ur   = tur;
		st.st_urev = turev;
	}
}

void
tween_pop(void)
{
	if (st.st_opts & OP_TWEEN) {
		tv = st.st_v;		st.st_v = sv;
		tlv = st.st_lv;		st.st_lv = slv;
		tur = st.st_ur;		st.st_ur = sur;
		turev = st.st_urev;	st.st_urev = surev;
	} else
		st.st_rf |= RF_CAM;
	tween_active = 0;
}

void
tween_toggle(void)
{
	tv = st.st_v;
	tlv = st.st_lv;
	tur = st.st_ur;
	turev = st.st_urev;
}

void
tween_getdst(struct fvec *v, struct fvec *lv, float *ur, int *urev)
{
	if (v)
		*v = tv;
	if (lv)
		*lv = tlv;
	if (ur)
		*ur = tur;
	if (urev)
		*urev = turev;
}

void
tween_setdst(const struct fvec *v, const struct fvec *lv, float ur, int urev)
{
	tv = *v;
	tlv = *lv;
	tur = ur;
	turev = urev;
}
