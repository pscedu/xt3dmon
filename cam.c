/* $Id$ */

#include "compat.h"

#include <math.h>

#include "cdefs.h"
#include "mon.h"
#include "xmath.h"

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

void
cam_revolve(struct fvec *center, float d_theta, float d_phi)
{
	struct fvec diff;
	float r2, r3, t, p;

	diff.fv_x = st.st_x - center->fv_x;
	diff.fv_y = st.st_y - center->fv_y;
	diff.fv_z = st.st_z - center->fv_z;

	r2 = sqrt(SQUARE(diff.fv_x) + SQUARE(diff.fv_z));
	r3 = vec_mag(&diff);
	t = acosf(diff.fv_x / r2);
	p = asinf(diff.fv_y / r3);
	if (st.st_z < center->fv_z)
		t = 2.0f * M_PI - t;
	t += 0.01f * d_theta;
	p += 0.01f * d_phi;
	while (t < 0)
		t += M_PI * 2.0f;

	st.st_x = r3 * cos(t) * cos(p);
	st.st_y = r3 * sin(p);
	st.st_z = r3 * sin(t) * cos(p);

	st.st_lx = -st.st_x;
	st.st_ly = -st.st_y;
	st.st_lz = -st.st_z;

	st.st_x += center->fv_x;
	st.st_y += center->fv_y;
	st.st_z += center->fv_z;

	vec_normalize(&st.st_lv);
}

/* XXX: amt only works when small - use fmod to get remainders of 2*PI. */
void
cam_roll(float amt)
{
	float t, mag;

	mag = vec_mag(&st.st_uv);
	t = acosf(st.st_uy / mag);
	if (st.st_uz < 0)
		t = 2.0f * M_PI - t;
	t += amt;
	if (t < 0)
		t += M_PI * 2.0f;
	st.st_uy = cos(t) * mag;
	st.st_uz = sin(t) * mag;
}

void
cam_rotate(float d_theta, float d_phi)
{
	float t, mag;

	if (d_theta) {
		mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_lz));
		t = acosf(st.st_lx / mag);
		if (st.st_lz < 0)
			t = 2.0f * M_PI - t;
		t += 0.01f * d_theta;
		if (t < 0)
			t += M_PI * 2.0f;
		st.st_lx = cos(t) * mag;
		st.st_lz = sin(t) * mag;
	}
	if (d_phi) {
		d_phi *= -0.005f;
		t = asinf(st.st_ly); /* XXX:  wrong. */
		if (t + d_phi < M_PI / 2.0f &&
		    t + d_phi > -M_PI / 2.0f) {
			st.st_ly = sin(t + d_phi);
			vec_normalize(&st.st_lv);
		}
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
