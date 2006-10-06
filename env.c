/* $Id$ */

#include "mon.h"

#include <math.h>
#include <time.h>

#include "cdefs.h"
#include "env.h"
#include "gl.h"
#include "node.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "xmath.h"

/* Stereo mode parameters. */
#define FOCAL_POINT	(2.00f) /* Distance from cam to 3d focus */
#define FOCAL_LENGTH	(5.00f) /* Length of 3D focus */
#define ST_EYESEP	(0.15f) /* Distance between "eyes" */

int		gl_cursor[2];		/* Current cursors for each stereo win */
int		cursor;			/* What the cursors will be */

struct fvec	focus;			/* 3D point of revolution */

struct ivec	winv = { { 800, 600, 0 } };

int	 	window_ids[2];
int	 	wid = WINID_DEF;	/* Current window ID */

struct keyh keyhtab[] = {
	{ "Camera movement",	gl_spkeyh_default },
	{ "Node neighbors",	gl_spkeyh_node },
	{ "Wired controls",	gl_spkeyh_wiadj }
};

__inline void
frustum_init(struct frustum *fr)
{
	fr->fr_ratio = ASPECT;
	fr->fr_radians = DEG_TO_RAD(FOVY / 4.0);
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
	case VM_PHYS:
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
	case VM_WIONE:
		cen->fv_x = st.st_winsp.iv_x * (widim.iv_w / 2.0f + st.st_wioff.iv_x);
		cen->fv_y = st.st_winsp.iv_y * (widim.iv_h / 2.0f + st.st_wioff.iv_y);
		cen->fv_z = st.st_winsp.iv_z * (widim.iv_d / 2.0f + st.st_wioff.iv_z);
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
		cen->fv_x += n->n_v->fv_x + n->n_dimp->fv_w / 2;
		cen->fv_y += n->n_v->fv_y + n->n_dimp->fv_h / 2;
		cen->fv_z += n->n_v->fv_z + n->n_dimp->fv_d / 2;

		/*
		 * In wired mode, add the offsets which determine
		 * in which wirep the node was selected in.
		 */
		if (st.st_vmode == VM_WIRED) {
			cen->fv_x += sn->sn_offv.fv_x;
			cen->fv_y += sn->sn_offv.fv_y;
			cen->fv_z += sn->sn_offv.fv_z;
		}
	}
	cen->fv_x /= nselnodes;
	cen->fv_y /= nselnodes;
	cen->fv_z /= nselnodes;
}

const char *caption;

__inline void
caption_set(const char *s)
{
	caption = s;
}

__inline const char *
caption_get(void)
{
	return (caption);
}

void
caption_setdrain(time_t t)
{
	static char capfmt[BUFSIZ], capbuf[BUFSIZ];
	struct tm tm;

	if (t) {
		localtime_r(&t, &tm);
		strftime(capfmt, sizeof(capfmt),
		    date_fmt, &tm);
		snprintf(capbuf, sizeof(capbuf),
		    "Drain set for %s", capfmt);
		caption_set(capbuf);
	} else
		caption_set(NULL);
}
