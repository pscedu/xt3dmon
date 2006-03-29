/* $Id$ */

#include "env.h"
#include "state.h"
#include "xmath.h"

#define FOCAL_POINT	(2.00f) /* distance from cam to center of 3d focus */
#define FOCAL_LENGTH	(5.00f) /* length of 3d focus */
#define ST_EYESEP	(0.30f) /* distance between "eyes" */

__inline void
frustum_init(struct frustum *fr)
{
	fr->fr_ratio = ASPECT;
	fr->fr_radians = DEG_TO_RAD(FOVY / 2);
	fr->fr_wd2 = FOCAL_POINT * tan(fr->fr_radians);
	fr->fr_ndfl = FOCAL_POINT / FOCAL_LENGTH;
	fr->fr_eyesep = ST_EYESEP;

	vec_crossprod(&fr->fr_stereov, &st.st_lv, &st.st_uv);
	vec_normalize(&fr->fr_stereov);
	fr->fr_stereov.fv_x *= fr->fr_eyesep / 2.0f;
	fr->fr_stereov.fv_y *= fr->fr_eyesep / 2.0f;
	fr->fr_stereov.fv_z *= fr->fr_eyesep / 2.0f;
}

__inline void
frustum_calc(int frid, struct frustum *fr)
{
	switch (frid) {
	case FRID_LEFT:
		fr->fr_left = -fr->fr_ratio * fr->fr_wd2 + fr->fr_eyesep * fr->fr_ndfl / 2;
		fr->fr_right = fr->fr_ratio * fr->fr_wd2 + fr->fr_eyesep * fr->fr_ndfl / 2;
		fr->fr_top = fr->fr_wd2;
		fr->fr_bottom = -fr->fr_wd2;
		break;
	case FRID_RIGHT:
		fr->fr_left = -fr->fr_ratio * fr->fr_wd2 - fr->fr_eyesep * fr->fr_ndfl / 2;
		fr->fr_right = fr->fr_ratio * fr->fr_wd2 - fr->fr_eyesep * fr->fr_ndfl / 2;
		fr->fr_top = fr->fr_wd2;
		fr->fr_bottom = -fr->fr_wd2;
		break;
	}
}
