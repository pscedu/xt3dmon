/* $Id$ */

#include <math.h>

#include "mon.h"

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
cam_move(int dir)
{
	float r, adj;

	r = sqrt(SQUARE(st.st_x - XCENTER) + SQUARE(st.st_z - ZCENTER));
	adj = pow(2, r / (ROWWIDTH / 2.0f));
	if (adj > 50.0f)
		adj = 50.0f;

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
}

void
cam_revolve(int d)
{
	float t, r, mag;
	struct vec v;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		v.v_x = XCENTER;
		v.v_y = YCENTER;
		v.v_z = ZCENTER;
		break;
	case VM_LOGICAL:
	case VM_LOGICALONE:
		v.v_x = st.st_x + st.st_lx * 300;
		v.v_y = st.st_y + st.st_ly * 300;
		v.v_z = st.st_z + st.st_lz * 300;
		break;
	}

	r = sqrt(SQUARE(st.st_x - v.v_x) + SQUARE(st.st_z - v.v_z));
	t = acosf((st.st_x - v.v_x) / r);
	if (st.st_z < v.v_z)
		t = 2.0f * PI - t;
	t += .01f * (float)d;
	if (t < 0)
		t += PI * 2.0f;

	/*
	 * Maintain the magnitude of lx*lx + lz*lz.
	 */
	mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_lz));
	st.st_x = r * cos(t) + v.v_x;
	st.st_z = r * sin(t) + v.v_z;
	st.st_lx = (v.v_x - st.st_x) / r * mag;
	st.st_lz = (v.v_z - st.st_z) / r * mag;
}

void
cam_rotateu(int d)
{
	float t, mag;

	mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_lz));
	t = acosf(st.st_lx / mag);
	if (st.st_lz < 0)
		t = 2.0f * PI - t;
	t += 0.01f * (float)d;
	if (t < 0)
		t += PI * 2.0f;
	st.st_lx = cos(t) * mag;
	st.st_lz = sin(t) * mag;
}

void
cam_rotatev(int d)
{
	float adj, t, mag;

	adj = d * -0.005f;
	t = asinf(st.st_ly); /* XXX:  wrong. */
	if (fabs(t + adj) < PI / 2.0f) {
		st.st_ly = sin(t + adj);
		mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_ly) +
		    SQUARE(st.st_lz));
		st.st_lx /= mag;
		st.st_ly /= mag;
		st.st_lz /= mag;
	}
}

void
cam_update(void)
{
	glLoadIdentity();
	gluLookAt(st.st_x, st.st_y, st.st_z,
	    st.st_x + st.st_lx,
	    st.st_y + st.st_ly,
	    st.st_z + st.st_lz,
	    0.0f, 1.0f, 0.0f);
}

void
tween_pushpop(int opts)
{
	static float sx, sy, sz, slx, sly, slz;

	if (opts & TWF_PUSH) {
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
	} else {
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
		} else
			cam_update();
	}
}
