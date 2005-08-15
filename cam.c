/* $Id$ */

#include "compat.h"

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
	case DIR_LEFT:
		st.st_x += st.st_lz * amt;
		st.st_z -= st.st_lx * amt;
		break;
	case DIR_RIGHT:
		st.st_x -= st.st_lz * amt;
		st.st_z += st.st_lx * amt;
		break;
	case DIR_FORWARD:
		st.st_x += st.st_lx * amt;
		st.st_y += st.st_ly * amt;
		st.st_z += st.st_lz * amt;
		break;
	case DIR_BACK:
		st.st_x -= st.st_lx * amt;
		st.st_y -= st.st_ly * amt;
		st.st_z -= st.st_lz * amt;
		break;
	case DIR_UP:
	case DIR_DOWN: {
		struct fvec cross, a, b;

		a.fv_x = st.st_lx;
		a.fv_y = st.st_ly;
		a.fv_z = st.st_lz;
		vec_normalize(&a);

		b.fv_x = st.st_lz;
		b.fv_y = 0;
		b.fv_z = -st.st_lx;
		vec_normalize(&b);

		/* Follow the normal to the look vector. */
		if (dir == DIR_DOWN)
			vec_crossprod(&cross, &b, &a);
		else
			vec_crossprod(&cross, &a, &b);

		vec_normalize(&cross);
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
	st.st_x = v->fv_x;
	st.st_y = v->fv_y;
	st.st_z = v->fv_z;
}

void
cam_revolve(int d)
{
	float t, r, mag, dist;
	struct fvec v;
	struct selnode *sn;

	/* Determine the center point to revolve around */
	if(nselnodes > 0 && spkey & GLUT_ACTIVE_ALT) {
		sn = SLIST_FIRST(&selnodes);

		v.fv_x = sn->sn_nodep->n_v->fv_x;
		v.fv_y = sn->sn_nodep->n_v->fv_y;
		v.fv_z = sn->sn_nodep->n_v->fv_z;

	} else {
		switch (st.st_vmode) {
		case VM_PHYSICAL:
			v.fv_x = XCENTER;
			v.fv_y = YCENTER;
			v.fv_z = ZCENTER;
			break;
		case VM_WIRED:
		case VM_WIREDONE:
			dist = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH);
			if (st.st_vmode == VM_WIRED)
				dist /= 3.0f;
			v.fv_x = st.st_x + st.st_lx * dist;
			v.fv_y = st.st_y + st.st_ly * dist;
			v.fv_z = st.st_z + st.st_lz * dist;
			break;
		}
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
	float adj, t;

	adj = d * -0.005f;
	t = asinf(st.st_ly); /* XXX:  wrong. */
	if (t + adj < PI / 2.0f &&
	    t + adj > -PI / 2.0f) {
		st.st_ly = sin(t + adj);
		vec_normalize(&st.st_lv);
	}
}

__inline void
cam_look(void)
{
	glLoadIdentity();
	gluLookAt(st.st_x, st.st_y, st.st_z,
	    st.st_x + st.st_lx,
	    st.st_y + st.st_ly,
	    st.st_z + st.st_lz,
	    st.st_uv.fv_x,
	    st.st_uv.fv_y,
	    st.st_uv.fv_z);
}
