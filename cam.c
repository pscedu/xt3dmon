/* $Id$ */

#include <math.h>

#include "mon.h"
#include "cam.h"

/*
 *    z
 *   / Forward /|
 *  /   Back  |/
 * +------------------- x
 * |  Left <-  -> Right
 * |
 * |   Up /\
 * | Down \/
 * y
 */
void
move_cam(int dir)
{
	float sx, sy, sz;
	float r, adj;

	r = sqrt(SQUARE(st.st_x - XCENTER) + SQUARE(st.st_z - ZCENTER));
	adj = pow(2, r / (ROWWIDTH / 2.0f));
	if (adj > 50.0f)
		adj = 50.0f;

	sx = sy = sz = 0.0f; /* gcc */
	if (st.st_opts & OP_TWEEN) {
		ox = st.st_x;  olx = st.st_lx;
		oy = st.st_y;  oly = st.st_ly;
		oz = st.st_z;  olz = st.st_lz;

		sx = st.st_x;  st.st_x = tx;
		sy = st.st_y;  st.st_y = ty;
		sz = st.st_z;  st.st_z = tz;
	}

	switch (dir) {
	case CAMDIR_LEFT:
		st.st_x += st.st_lz * 0.3f * SCALE * adj;
		st.st_z -= st.st_lx * 0.3f * SCALE * adj;
		break;
	case CAMDIR_RIGHT:
		st.st_x -= st.st_lz * 0.3f * SCALE * adj;
		st.st_z += st.st_lx * 0.3f * SCALE * adj;
		break;
	case CAMDIR_FORWARD:
		st.st_x += st.st_lx * 0.3f * SCALE * adj;
		st.st_y += st.st_ly * 0.3f * SCALE * adj;
		st.st_z += st.st_lz * 0.3f * SCALE * adj;
		break;
	case CAMDIR_BACK:
		st.st_x -= st.st_lx * 0.3f * SCALE * adj;
		st.st_y -= st.st_ly * 0.3f * SCALE * adj;
		st.st_z -= st.st_lz * 0.3f * SCALE * adj;
		break;
	case CAMDIR_UP:
		st.st_x += st.st_lx * st.st_ly * 0.3f * SCALE * adj;
		st.st_y += 0.3f * SCALE;
		st.st_z += st.st_lz * st.st_ly * 0.3f * SCALE * adj;
		break;
	case CAMDIR_DOWN:
		st.st_x -= st.st_lx * st.st_ly * 0.3f * SCALE * adj;
		st.st_y -= 0.3f * SCALE;
		st.st_z -= st.st_lz * st.st_ly * 0.3f * SCALE * adj;
		break;
	}

	if (st.st_opts & OP_TWEEN) {
		tx = st.st_x;  st.st_x = sx;
		ty = st.st_y;  st.st_y = sy;
		tz = st.st_z;  st.st_z = sz;
	} else
		adjcam();
}

void
rotate_cam(int du, int dv)
{
	float sx, sy, sz, slx, sly, slz;
	float adj, t, r, mag;

	sx = sy = sz = 0.0f; /* gcc */
	slx = sly = slz = 0.0f; /* gcc */
	if (st.st_opts & OP_TWEEN) {
		ox = st.st_x;  olx = st.st_lx;
		oy = st.st_y;  oly = st.st_ly;
		oz = st.st_z;  olz = st.st_lz;

		sx = st.st_x;  st.st_x = tx;
		sy = st.st_y;  st.st_y = ty;
		sz = st.st_z;  st.st_z = tz;

		slx = st.st_lx;  st.st_lx = tlx;
		sly = st.st_ly;  st.st_ly = tly;
		slz = st.st_lz;  st.st_lz = tlz;
	}

	if (du != 0 && spkey & GLUT_ACTIVE_CTRL) {
		r = sqrt(SQUARE(st.st_x - XCENTER) +
		    SQUARE(st.st_z - ZCENTER));
		t = acosf((st.st_x - XCENTER) / r);
		if (st.st_z < ZCENTER)
			t = 2.0f * PI - t;
		t += .01f * (float)du;
		if (t < 0)
			t += PI * 2.0f;

		/*
		 * Maintain the magnitude of lx*lx + lz*lz.
		 */
		mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_lz));
		st.st_x = r * cos(t) + XCENTER;
		st.st_z = r * sin(t) + ZCENTER;
		st.st_lx = (XCENTER - st.st_x) / r * mag;
		st.st_lz = (ZCENTER - st.st_z) / r * mag;
	}
	if (dv != 0 && spkey & GLUT_ACTIVE_SHIFT) {
		adj = (dv < 0) ? 0.005f : -0.005f;
		t = asinf(st.st_ly);
		if (fabs(t + adj) < PI / 2.0f) {
			st.st_ly = sin(t + adj);
			mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_ly) +
			    SQUARE(st.st_lz));
			st.st_lx /= mag;
			st.st_ly /= mag;
			st.st_lz /= mag;
		}
	}

	if (st.st_opts & OP_TWEEN) {
		tx = st.st_x;  st.st_x = sx;
		ty = st.st_y;  st.st_y = sy;
		tz = st.st_z;  st.st_z = sz;

		tlx = st.st_lx;  st.st_lx = slx;
		tly = st.st_ly;  st.st_ly = sly;
		tlz = st.st_lz;  st.st_lz = slz;
	} else
		adjcam();
}
