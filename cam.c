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
