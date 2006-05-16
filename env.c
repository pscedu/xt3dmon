/* $Id$ */

#include "cdefs.h"
#include "env.h"
#include "gl.h"
#include "node.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "xmath.h"

#define FOCAL_POINT	(2.00f) /* distance from cam to center of 3d focus */
#define FOCAL_LENGTH	(5.00f) /* length of 3d focus */
#define ST_EYESEP	(0.30f) /* distance between "eyes" */

int cursor;
int gl_cursor[2];

struct fvec focus;

struct ivec	 winv = { 800, 600, 0 };

int	 window_ids[2];
int	 wid = WINID_DEF;		/* current window */

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

void
cursor_update(void)
{
	if (gl_cursor[wid] != cursor) {
		glutSetCursor(cursor);
		gl_cursor[wid] = cursor;
	}
}

void
cursor_set(int curs)
{
	cursor = curs;
	gl_run(cursor_update);
}

void
focus_cluster(struct fvec *cen)
{
	double dist;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		cen->fv_x = XCENTER;
		cen->fv_y = YCENTER;
		cen->fv_z = ZCENTER;
		break;
	case VM_WIRED:
		dist = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH) / 4.0;
		cen->fv_x = st.st_x + st.st_lx * dist;
		cen->fv_y = st.st_y + st.st_ly * dist;
		cen->fv_z = st.st_z + st.st_lz * dist;
		break;
	case VM_WIREDONE:
		cen->fv_x = st.st_winsp.iv_x * (WIDIM_WIDTH  / 2.0f + st.st_wioff.iv_x);
		cen->fv_y = st.st_winsp.iv_y * (WIDIM_HEIGHT / 2.0f + st.st_wioff.iv_y);
		cen->fv_z = st.st_winsp.iv_z * (WIDIM_DEPTH  / 2.0f + st.st_wioff.iv_z);
		break;
	}
}

void
focus_selnodes(struct fvec *cen)
{
	struct selnode *sn;
	struct node *n;

	vec_set(cen, 0.0, 0.0, 0.0);
	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;

		switch (st.st_vmode) {
		case VM_WIRED:
			break;
		}

		cen->fv_x += n->n_v->fv_x + n->n_dimp->fv_w / 2;
		cen->fv_y += n->n_v->fv_y + n->n_dimp->fv_h / 2;
		cen->fv_z += n->n_v->fv_z + n->n_dimp->fv_d / 2;
	}
	cen->fv_x /= nselnodes;
	cen->fv_y /= nselnodes;
	cen->fv_z /= nselnodes;
}
