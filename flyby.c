/* $Id$ */

#include "mon.h"

/*
 * struct state {
 *	float		st_x;
 *	float		st_y;
 *	float		st_z;
 *	float		st_lx;
 *	float		st_ly;
 *	float		st_lz;
 *	int		st_opts;
 *	int		st_panels;
 *	float		st_alpha_job;
 *	float		st_alpha_oth;
 *	GLint		st_alpha_fmt;
 *
 *	int		st_tween_mode;
 *	int		st_nframes;
 * };
 */

const struct state flybypath[] = {
 /*      x        y        z      lx      ly      lz  options                          panels  ajob  aoth  afmt     tween_mode   nframes */
 {  50.00f,  20.00f,  10.00f, -0.40f,  0.10f,  0.40f, OP_WIRES | OP_TWEEN | OP_GROUND, 0,      1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 100 },
 { -10.00f,  80.00f,  12.00f,  0.60f, -0.15f,  0.50f, OP_WIRES | OP_TWEEN | OP_GROUND, 0,      1.0f, 1.0f, GL_RGBA, TM_RADIUS,   100 },
 {   0.00f,   0.00f,   0.00f,  0.00f,  0.00f,  0.00f, 0,                               0,      0.0f, 0.0f, 0,       0,           0   }
};
