/* $Id$ */

#include "mon.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "cdefs.h"
#include "cam.h"
#include "env.h"
#include "lnseg.h"
#include "mark.h"
#include "deusex.h"
#include "state.h"
#include "tween.h"

int dx_orbit_u(void);
int dx_orbit_urev(void);
int dx_orbit_v(void);
int dx_orbit_vrev(void);
int dx_cubanoid_x(void);
int dx_cubanoid_y(void);
int dx_cubanoid_z(void);
int dx_corkscrew_x(void);
int dx_orbit3_u(void);
int dx_orbit3_urev(void);
int dx_orbit3_v(void);
int dx_orbit3_vrev(void);

void dxi_orbit(void);

struct dxte {
	void (*de_init)(void);
	int  (*de_update)(void);
} dxtab[] = {
	{ dxi_orbit, dx_orbit_u },
	{ dxi_orbit, dx_orbit_urev },
	{ dxi_orbit, dx_orbit_v },
	{ dxi_orbit, dx_orbit_vrev },
//	{ NULL, dx_cubanoid_x },
	{ NULL, dx_cubanoid_y },
//	{ NULL, dx_cubanoid_z },
//	{ dxi_orbit, dx_orbit3_u },
//	{ dxi_orbit, dx_orbit3_urev },
//	{ dxi_orbit, dx_orbit3_v },
//	{ dxi_orbit, dx_orbit3_vrev },
	{ NULL, dx_corkscrew_x }
};
#define NENTRIES(t) (sizeof((t)) / sizeof((t)[0]))

void
dx_update(void)
{
	static struct dxte *de;

	if (de == NULL) {
		de = &dxtab[rand() % NENTRIES(dxtab)];
		if (de->de_init != NULL)
			de->de_init();
	}

	if (de->de_update())
		de = NULL;
}

#define DST(a, b)						\
	(sqrt(						\
	    SQUARE((a)->fv_x - (b)->fv_x) +		\
	    SQUARE((a)->fv_y - (b)->fv_y) +		\
	    SQUARE((a)->fv_z - (b)->fv_z)))

void
dxi_orbit(void)
{
	double rx, ry, rz;

	tween_push(TWF_POS);
	do {
		rx = (rand() % 2 ? 1 : -1) * fmod(rand() / 100.0, ROWWIDTH);
		ry = (rand() % 2 ? 1 : -1) * fmod(rand() / 100.0, ROWWIDTH);
		rz = (rand() % 2 ? 1 : -1) * fmod(rand() / 100.0, ROWWIDTH);

		st.st_v = focus;
		st.st_x += rx;
		st.st_y += ry;
		st.st_z += rz;
	} while (DST(&focus, &st.st_v) < ROWDEPTH);
	tween_pop(TWF_POS);
}

int
dx_orbit(int dim, int sign)
{
	static double t;
	double du, dv;
	int ret;

	ret = 0;
	du = dv = 0.0;
	switch (dim) {
	case DIM_X:
		du = sign * 2.0;
		break;
	case DIM_Y:
		dv = sign * -2.0;
		break;
	}

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);
	cam_revolve(&focus, du, dv);
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += M_PI / 167;
	if (t >= 2.0 * M_PI) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_orbit_u(void)
{
	return (dx_orbit(DIM_X, 1));
}

int
dx_orbit_urev(void)
{
	return (dx_orbit(DIM_X, -1));
}

int
dx_orbit_v(void)
{
	return (dx_orbit(DIM_Y, 1));
}

int
dx_orbit_vrev(void)
{
	return (dx_orbit(DIM_Y, -1));
}

int
dx_orbit3(int dim, int sign)
{
	static int n = 0;
	int ret;

	if (dx_orbit(DIM_Y, sign))
		n++;

	ret = 0;
	if (n >= 3) {
		n = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_orbit3_u(void)
{
	return (dx_orbit3(DIM_X, 1));
}

int
dx_orbit3_urev(void)
{
	return (dx_orbit3(DIM_X, -1));
}

int
dx_orbit3_v(void)
{
	return (dx_orbit3(DIM_Y, 1));
}

int
dx_orbit3_vrev(void)
{
	return (dx_orbit3(DIM_Y, -1));
}

int
dx_cubanoid(int dim)
{
	static double t;
	double roll, a, b, max;
	struct fvec sv, uv, lv, xv;
	int ret;

	ret = 0;

	switch (dim) {
	case DIM_X:
		a = CABHEIGHT/4.0;
		b = ROWDEPTH/4.0;
		break;
	case DIM_Y:
		a = ROWWIDTH/4.0;
		b = CABHEIGHT/2.0;
		break;
	case DIM_Z:
		a = ROWWIDTH/4.0 - CABWIDTH;
		b = CABHEIGHT/4.0;
		break;
	}
	max = MAX(a, b);

	if (t < 2 * M_PI) {
		sv.fv_x = a * sin(t - M_PI/2) + max;
		sv.fv_y = b * cos(t - M_PI/2);

		uv.fv_x = sin(t - M_PI/2);
		uv.fv_y = cos(t - M_PI/2);
	} else {
		sv.fv_x = -a * sin(t - M_PI/2) - max;
		sv.fv_y = b * cos(t - M_PI/2);

		uv.fv_x = sin(t - M_PI/2);
		uv.fv_y = -cos(t - M_PI/2);
	}
	sv.fv_z = 0.0;
	uv.fv_z = 0.0;

	vec_set(&xv, 0.0, 0.0, 1.0);
	vec_normalize(&uv);
	vec_crossprod(&lv, &uv, &xv);

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);

	switch (dim) {
	case DIM_X:
		st.st_x = XCENTER;
		st.st_y = YCENTER + sv.fv_x;
		st.st_z = ZCENTER + sv.fv_y;

		st.st_ux = 0.0;
		st.st_uy = uv.fv_x;
		st.st_uz = uv.fv_y;

		st.st_lx = 0.0;
		st.st_ly = lv.fv_x;
		st.st_lz = lv.fv_y;
		break;
	case DIM_Y:
		st.st_x = XCENTER + sv.fv_x;
		st.st_y = YCENTER;
		st.st_z = ZCENTER + sv.fv_y;

		st.st_ux = uv.fv_x;
		st.st_uy = 0.0;
		st.st_uz = uv.fv_y;

		st.st_lx = lv.fv_x;
		st.st_ly = 0.0;
		st.st_lz = lv.fv_y;
		break;
	case DIM_Z:
		st.st_x = XCENTER + sv.fv_x;
		st.st_y = YCENTER + sv.fv_y;
		st.st_z = ZCENTER;

		st.st_ux = uv.fv_x;
		st.st_uy = uv.fv_y;
		st.st_uz = 0.0;

		st.st_lx = lv.fv_x;
		st.st_ly = lv.fv_y;
		st.st_lz = 0.0;
		break;
	}

	roll = 0.0;
	if (t > M_PI / 2.0 && t < 3 * M_PI / 2.0)
		roll = t - M_PI / 2.0;
	else if (t > 3 * M_PI / 2.0 && t < 5 * M_PI/2.0)
		roll = M_PI;
	else if (t > 5 * M_PI / 2.0 && t < 7 * M_PI / 2.0)
		roll = M_PI + t - 5 * M_PI / 2.0;

	while (roll > 0.1) {
		vec_rotate(&st.st_uv, &st.st_lv, 0.1);
		roll -= 0.1;
	}

	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += 0.025;
	if (t > 4 * M_PI) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_cubanoid_x(void)
{
	return (dx_cubanoid(DIM_X));
}

int
dx_cubanoid_y(void)
{
	return (dx_cubanoid(DIM_Y));
}

int
dx_cubanoid_z(void)
{
	return (dx_cubanoid(DIM_Z));
}

int
dx_corkscrew(int dim)
{
	static double t;
	double a, b, c;
	struct fvec sv;
	int ret;

	ret = 0;
	switch (dim) {
	case DIM_X:
		a = CABHEIGHT / 4.0;
		b = ((NROWS * ROWDEPTH + (NROWS - 1) * ROWSPACE)) / 4.0;
		c = ROWWIDTH;
		break;
	case DIM_Y:
		a = ROWWIDTH;
		b = ROWDEPTH;
		c = CABHEIGHT;
		break;
	case DIM_Z:
		a = ROWWIDTH;
		b = CABHEIGHT;
		c = ROWDEPTH;
		break;
	}

	sv.fv_x = t;
	sv.fv_y = a * sin(t / c * 4.0 * M_PI);
	sv.fv_z = b * cos(t / c * 4.0 * M_PI);

	tween_push(TWF_LOOK | TWF_POS | TWF_UP);

	switch (dim) {
	case DIM_X:
		st.st_x = t;
		st.st_y = YCENTER + sv.fv_y;
		st.st_z = ZCENTER + sv.fv_z;

		if (t == 0.0) {
			vec_set(&st.st_uv, 0.0, 1.0, 0.0);
			vec_set(&st.st_lv, 1.0, 0.0, 0.0);
		}
		break;
	case DIM_Y:
		st.st_x = XCENTER + sv.fv_y;
		st.st_y = t;
		st.st_z = ZCENTER + sv.fv_z;

		if (t == 0.0) {
			vec_set(&st.st_uv, 0.0, 0.0, 1.0);
			vec_set(&st.st_lv, 0.0, 1.0, 0.0);
		}
		break;
	case DIM_Z:
		st.st_x = XCENTER + sv.fv_y;
		st.st_y = YCENTER + sv.fv_z;
		st.st_z = t;

		if (t == 0.0) {
			vec_set(&st.st_uv, 0.0, 1.0, 0.0);
			vec_set(&st.st_lv, 0.0, 0.0, 1.0);
		}
		break;
	}

	vec_rotate(&st.st_uv, &st.st_lv, 2.0 * M_PI / c);
	tween_pop(TWF_LOOK | TWF_POS | TWF_UP);

	t += 0.5;
	if (t > c) {
		t = 0;
		ret = 1;
	}
	return (ret);
}

int
dx_corkscrew_x(void)
{
	return (dx_corkscrew(DIM_X));
}

int
dx_corkscrew_y(void)
{
	return (dx_corkscrew(DIM_Y));
}

int
dx_corkscrew_z(void)
{
	return (dx_corkscrew(DIM_Z));
}
