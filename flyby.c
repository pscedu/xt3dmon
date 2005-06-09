/* $Id$ */

#include "mon.h"
#include<stdio.h>
#include<err.h>

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

#if 0
const struct state flybypath[] = {
 /*      x        y        z      lx      ly      lz  options                            panels       ajob  aoth  afmt     tween_mode   nframes */
#if 0
 {  50.00f,  20.00f,  10.00f, -0.40f,  0.10f,  0.40f, OP_WIRES | OP_CAPTURE | OP_DISPLAY, 0,           1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 200 },
 { -10.00f,   8.00f,  12.00f,  0.60f, -0.15f,  0.50f, OP_WIRES | OP_CAPTURE | OP_DISPLAY, 0,           1.0f, 1.0f, GL_RGBA, TM_RADIUS,   200 },
 { 238.29f,  26.90f, -28.59f, -0.68f, -0.15f,  0.38f, OP_WIRES | OP_CAPTURE | OP_DISPLAY, PANEL_FPS,   1.0f, 1.0f, GL_RGBA, TM_RADIUS,   500 },
 { 136.09f,   6.21f,   8.50f, -0.78f, -0.15f,  0.02f, OP_WIRES | OP_CAPTURE | OP_DISPLAY, PANEL_NINFO, 1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 100 },
 {   0.00f,   0.00f,   0.00f,  0.00f,  0.00f,  0.00f, 0,                                 0,           0.0f, 0.0f, 0,       0,           0   }
};
 #endif

#if 0
 {  50.00f,  20.00f,  10.00f, -0.40f,  0.10f,  0.40f, OP_WIRES | OP_CAPTURE | OP_TEX | OP_BLEND, 0,           1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 200 },
 { -10.00f,   8.00f,  12.00f,  0.60f, -0.15f,  0.50f, OP_WIRES | OP_CAPTURE | OP_TEX | OP_BLEND, 0,           1.0f, 1.0f, GL_RGBA, TM_RADIUS,   200 },
 { 238.29f,  26.90f, -28.59f, -0.68f, -0.15f,  0.38f, OP_WIRES | OP_CAPTURE | OP_TEX | OP_BLEND, PANEL_FPS,   1.0f, 1.0f, GL_RGBA, TM_RADIUS,   500 },
 { 136.09f,   6.21f,   8.50f, -0.78f, -0.15f,  0.02f, OP_WIRES | OP_CAPTURE | OP_TEX | OP_BLEND, PANEL_NINFO, 1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 100 },
 {   0.00f,   0.00f,   0.00f,  0.00f,  0.00f,  0.00f, 0,                                 0,           0.0f, 0.0f, 0,       0,           0   }
};
#endif

 {  50.00f,  20.00f,  10.00f, -0.40f,  0.10f,  0.40f, OP_WIRES | OP_DISPLAY, 0,           1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 200 },
 { -10.00f,   8.00f,  12.00f,  0.60f, -0.15f,  0.50f, OP_WIRES | OP_DISPLAY, 0,           1.0f, 1.0f, GL_RGBA, TM_RADIUS,   200 },
 { 238.29f,  26.90f, -28.59f, -0.68f, -0.15f,  0.38f, OP_WIRES | OP_DISPLAY, PANEL_FPS,   1.0f, 1.0f, GL_RGBA, TM_RADIUS,   500 },
 { 136.09f,   6.21f,   8.50f, -0.78f, -0.15f,  0.02f, OP_WIRES | OP_DISPLAY, PANEL_NINFO, 1.0f, 1.0f, GL_RGBA, TM_STRAIGHT, 100 },
 {   0.00f,   0.00f,   0.00f,  0.00f,  0.00f,  0.00f, 0,                                 0,           0.0f, 0.0f, 0,       0,           0   }
};
#endif


#define FLYBY_FILE "flyby.data"
static FILE *flyby_fp;

/* Open the flyby data file appropriately */
void begin_flyby(char m)
{
	if(flyby_fp != NULL)
		return;

	if(m == 'r')
	{
		if((flyby_fp = fopen(FLYBY_FILE, "rb")) == NULL)
			err(1, "flyby data file err");
	} else if (m == 'w') {
		if((flyby_fp = fopen(FLYBY_FILE, "ab")) == NULL)
			err(1, "flyby data file err");
	}
}

/* Write current data for flyby */
void write_flyby()
{
	if(!fwrite(&st, sizeof(struct state), 1, flyby_fp))
		err(1, "flyby data write err");
}

/* Read a set of flyby data */
void read_flyby()
{
	if(!fread(&st, sizeof(struct state), 1, flyby_fp)) {

		/* end of flyby */
		if(feof(flyby_fp) != 0) {
			active_flyby = 0;
			glutKeyboardFunc(keyh_default);
			glutSpecialFunc(spkeyh_default);
		}
		else
			err(1, "flyby data read err");
	}

	restore_state();
}

void end_flyby()
{
	if(flyby_fp != NULL) {
		fclose(flyby_fp);
		flyby_fp = NULL;
	}
}




