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
 /*      x        y        z      lx      ly      lz  options                          panels       ajob  aoth  afmt     tween_mode   nframes */
 {  50.00f,  20.00f,  10.00f, -0.40f,  0.10f,  0.40f, OP_WIRES | /* OP_TWEEN | */ OP_GROUND, 0,           1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 200 },
 { -10.00f,   8.00f,  12.00f,  0.60f, -0.15f,  0.50f, OP_WIRES | /* OP_TWEEN | */ OP_GROUND, 0,           1.0f, 1.0f, GL_RGBA, TM_RADIUS,   200 },
 { 238.29f,  26.90f, -28.59f, -0.68f, -0.15f,  0.38f, OP_WIRES | /* OP_TWEEN | */ OP_GROUND, PANEL_FPS,   1.0f, 1.0f, GL_RGBA, TM_RADIUS,   500 },
 { 136.09f,   6.21f,   8.50f, -0.78f, -0.15f,  0.02f, OP_WIRES | /* OP_TWEEN | */ OP_GROUND, PANEL_NINFO, 1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 100 },
 {   0.00f,   0.00f,   0.00f,  0.00f,  0.00f,  0.00f, 0,                               0,           0.0f, 0.0f, 0,       0,           0   }
};
