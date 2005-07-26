/* $Id$ */

#include <math.h>

#include "cdefs.h"
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
cam_move(int dir, float amt)
{
	switch (dir) {
	case CAMDIR_LEFT:
		st.st_x += st.st_lz * amt;
		st.st_z -= st.st_lx * amt;
		break;
	case CAMDIR_RIGHT:
		st.st_x -= st.st_lz * amt;
		st.st_z += st.st_lx * amt;
		break;
	case CAMDIR_FORWARD:
		st.st_x += st.st_lx * amt;
		st.st_y += st.st_ly * amt;
		st.st_z += st.st_lz * amt;
		break;
	case CAMDIR_BACK:
		st.st_x -= st.st_lx * amt;
		st.st_y -= st.st_ly * amt;
		st.st_z -= st.st_lz * amt;
		break;
	case CAMDIR_UP:
	case CAMDIR_DOWN: {
		struct fvec cross, a, b, t;
		float mag;

		a.fv_x = st.st_lx;
		a.fv_y = st.st_ly;
		a.fv_z = st.st_lz;
		mag = sqrt(SQUARE(a.fv_x) + SQUARE(a.fv_y) +
		    SQUARE(a.fv_z));
		a.fv_x /= mag;
		a.fv_y /= mag;
		a.fv_z /= mag;

		b.fv_x = st.st_lz;
		b.fv_y = 0;
		b.fv_z = -st.st_lx;
		mag = sqrt(SQUARE(b.fv_x) + SQUARE(b.fv_y) +
		    SQUARE(b.fv_z));
		b.fv_x /= mag;
		b.fv_y /= mag;
		b.fv_z /= mag;

		if (dir == CAMDIR_DOWN)
			SWAP(a, b, t);

		/* Follow the normal to the look vector. */
		cross.fv_x = a.fv_y * b.fv_z - b.fv_y * a.fv_z;
		cross.fv_y = b.fv_x * a.fv_z - a.fv_x * b.fv_z;
		cross.fv_z = a.fv_x * b.fv_y - b.fv_x * a.fv_y;

		mag = sqrt(SQUARE(cross.fv_x) + SQUARE(cross.fv_y) +
		    SQUARE(cross.fv_z));
		cross.fv_x /= mag;
		cross.fv_y /= mag;
		cross.fv_z /= mag;

		st.st_x += cross.fv_x * amt;
		st.st_y += cross.fv_y * amt;
		st.st_z += cross.fv_z * amt;
		break;
	    }
	}
}

/*
 * Have the camera "go to" a specific place.
 * Considerations:
 *	- respect the shortest path there
 *	- respect wild camera angle tweening
 *	- respect obstructing objects in the viewing frame
 */
void
cam_goto(struct fvec *v)
{
	if (st.st_opts & OP_TWEEN) {
		tx = v->fv_x;
		ty = v->fv_y;
		tz = v->fv_z;
	} else {
		st.st_x = v->fv_x;
		st.st_y = v->fv_y;
		st.st_z = v->fv_z;
	}
}

void
cam_revolve(int d)
{
	float t, r, mag;
	struct fvec v;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		v.fv_x = XCENTER;
		v.fv_y = YCENTER;
		v.fv_z = ZCENTER;
		break;
	case VM_WIRED:
	case VM_WIREDONE:
		v.fv_x = st.st_x + st.st_lx * 100;
		v.fv_y = st.st_y + st.st_ly * 100;
		v.fv_z = st.st_z + st.st_lz * 100;
		break;
	}

	r = sqrt(SQUARE(st.st_x - v.fv_x) + SQUARE(st.st_z - v.fv_z));
	t = acosf((st.st_x - v.fv_x) / r);
	if (st.st_z < v.fv_z)
		t = 2.0f * PI - t;
	t += .01f * (float)d;
	if (t < 0)
		t += PI * 2.0f;

	/*
	 * Maintain the magnitude of lx*lx + lz*lz.
	 */
	mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_lz));
	st.st_x = r * cos(t) + v.fv_x;
	st.st_z = r * sin(t) + v.fv_z;
	st.st_lx = (v.fv_x - st.st_x) / r * mag;
	st.st_lz = (v.fv_z - st.st_z) / r * mag;
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
	if (t + adj < PI / 2.0f &&
	    t + adj > -PI / 2.0f) {
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
