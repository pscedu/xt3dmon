/* $Id$ */

#include <sys/time.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <math.h>
#include <stdio.h>

#include "mon.h"

void
draw(void)
{
	/* record user commands */
	if(build_flyby)
		write_flyby();

	if (active_flyby) {
		//calc_flyby();
		read_flyby();
//		restore_state();
	}

	if (st.st_opts & OP_TWEEN &&
	    (tx - st.st_x || ty - st.st_y || tz - st.st_z ||
	    tlx - st.st_lx || tly - st.st_ly || tlz - st.st_lz)) {
		st.st_x += (tx - st.st_x) * TWEEN_AMT;
		st.st_y += (ty - st.st_y) * TWEEN_AMT;
		st.st_z += (tz - st.st_z) * TWEEN_AMT;
		if (fabs(tx - st.st_x) < TWEEN_THRES)
			st.st_x = tx;
		if (fabs(ty - st.st_y) < TWEEN_THRES)
			st.st_y = ty;
		if (fabs(tz - st.st_z) < TWEEN_THRES)
			st.st_z = tz;

		st.st_lx += (tlx - st.st_lx) * TWEEN_AMT;
		st.st_ly += (tly - st.st_ly) * TWEEN_AMT;
		st.st_lz += (tlz - st.st_lz) * TWEEN_AMT;
		if (fabs(tlx - st.st_lx) < TWEEN_THRES)
			st.st_lx = tlx;
		if (fabs(tly - st.st_ly) < TWEEN_THRES)
			st.st_ly = tly;
		if (fabs(tlz - st.st_lz) < TWEEN_THRES)
			st.st_lz = tlz;
		adjcam();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (st.st_opts & OP_GROUND) {
#if 0
		float sx = -13.0;
		float w = 256;
		float d = 256;
		float sz = -128.0;
		float f = 20.0;
//		float t = 128;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 5);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBegin(GL_QUADS);
		glVertex3f(sx+w, 0.0f, sz);
		glTexCoord3f(0.0, 0.0, 1.0);
		glVertex3f(sx, 0.0f, sz);
		glTexCoord3f(0.0, f, 1.0);
		glVertex3f(sx, 0.0f, sz+d);
		glTexCoord3f(f, f, 1.0);
		glVertex3f(sx+w, 0.0f, sz+d);
		glTexCoord3f(f, 0.0, 1.0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
#endif

		/* Ground */
		glColor3f(0.4f, 0.4f, 0.4f);
		glBegin(GL_QUADS);
		glVertex3f( -5.0f, 0.0f, -5.0f);
		glVertex3f( -5.0f, 0.0f, 22.0f);
		glVertex3f(230.0f, 0.0f, 22.0f);
		glVertex3f(230.0f, 0.0f, -5.0f);
		glEnd();

		/* x-axis */
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
		glVertex2f(-200.0f, -0.1f);
		glVertex2f(-200.0f,  0.1f);
		glVertex2f( 200.0f,  0.1f);
		glVertex2f( 200.0f, -0.1f);
		glEnd();

		/* y-axis */
		glColor3f(0.6f, 0.6f, 1.0f);
		glBegin(GL_QUADS);
		glVertex2f(-0.1f, -200.0f);
		glVertex2f(-0.1f,  200.0f);
		glVertex2f( 0.1f,  200.0f);
		glVertex2f( 0.1f, -200.0f);
		glEnd();

		/* z-axis */
		glColor3f(1.0f, 0.9f, 0.0f);
		glBegin(GL_QUADS);
		glVertex3f(-0.1f, -0.1f, -200.0f);
		glVertex3f(-0.1f,  0.1f, -200.0f);
		glVertex3f(-0.1f,  0.1f,  200.0f);
		glVertex3f(-0.1f, -0.1f,  200.0f);
		glVertex3f( 0.1f, -0.1f, -200.0f);
		glVertex3f( 0.1f,  0.1f, -200.0f);
		glVertex3f( 0.1f,  0.1f,  200.0f);
		glVertex3f( 0.1f, -0.1f,  200.0f);
		glEnd();
	}

	draw_panels();

	glCallList(cluster_dl);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	if (st.st_opts & OP_CAPTURE)
		capture_fb();
	if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();
}

/*
 * y
 * |        (x,y+height,z+depth) (x+width,y+height,z+depth)
 * |                      +-------------+
 * |                (11) /      10     /|
 * |                    / |           / |
 * |                   /     top  15 /  |
 * |                6 /   |         /   |
 * |                 /      12     /    |
 * | (x,y+height,z) +-------------+ (x+width,y+height,z)
 * |                |             |     |
 * |                |     |       |     | front
 * |                |      (right)|     |
 * |                |   7 |       |     | 9
 * |                |             |(14) |
 * |         (back) |     |       |     |
 * |                |    left     | 13  |
 * |              5 |     |       |     |
 * |                |         2   |     |
 * |                |     + - - - | - - + (x+width,y,z+depth)
 * |  (x,y,z+depth) |    /   (8)  |    /
 * |                | 1 /         |   /
 * |     z          |  / (bottom) |  / 3
 * |    /           | /           | /
 * |   /            |/     4      |/
 * |  /             +-------------+
 * | /           (x,y,z)    (x+width,y,z)
 * |/
 * +--------------------------------------------------- x
 * (0,0,0)
 */

__inline void
draw_filled_node(struct node *n, float w, float h, float d)
{
	float x = n->n_pos.np_x;
	float y = n->n_pos.np_y;
	float z = n->n_pos.np_z;
	float r, g, b, a;

	switch (st.st_mode) {
	case SM_JOBS:
		if (n->n_state == JST_USED) {
			r = n->n_job->j_fill.f_r;
			g = n->n_job->j_fill.f_g;
			b = n->n_job->j_fill.f_b;
			a = st.st_alpha_job;
		} else {
			r = jstates[n->n_state].js_fill.f_r;
			g = jstates[n->n_state].js_fill.f_g;
			b = jstates[n->n_state].js_fill.f_b;
			a = st.st_alpha_oth;
		}
		break;
	case SM_TEMP:
		break;
	case SM_FAIL:
		break;
	}

	if (st.st_opts & OP_BLEND) {
		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
	} else
		a = 1.0;

	glColor4f(r, g, b, a);
	glBegin(GL_POLYGON);
	/* Bottom */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+d);		/*  1 */
	glVertex3f(x+w, y, z+d);	/*  2 */
	glVertex3f(x+w, y, z);		/*  3 */
	glVertex3f(x, y, z);		/*  4 */
	/* Back */
	glVertex3f(x, y+h, z);		/*  5 */
	glVertex3f(x, y+h, z+d);	/*  6 */
	glVertex3f(x, y, z+d);		/*  7 */
	/* Right */
	glVertex3f(x+w, y, z+d);	/*  8 */
	glVertex3f(x+w, y+h, z+d);	/*  9 */
	glVertex3f(x, y+h, z+d);	/* 10 */

	glVertex3f(x, y+h, z);		/* 11 */

	/* Left */
	glVertex3f(x+w, y+h, z);	/* 12 */
	glVertex3f(x+w, y, z);		/* 13 */
	glVertex3f(x+w, y+h, z);	/* 14 */
	/* Front */
	glVertex3f(x+w, y+h, z+d);	/* 15 */
	glEnd();

	if (st.st_opts & OP_BLEND)
		glDisable(GL_BLEND);
}

__inline void
draw_wireframe_node(struct node *n, float w, float h, float d)
{
	float x = n->n_pos.np_x;
	float y = n->n_pos.np_y;
	float z = n->n_pos.np_z;

	/* Wireframe outline */
	x -= WFRAMEWIDTH;
	y -= WFRAMEWIDTH;
	z -= WFRAMEWIDTH;
	w += 2.0f * WFRAMEWIDTH;
	h += 2.0f * WFRAMEWIDTH;
	d += 2.0f * WFRAMEWIDTH;

	glLineWidth(1.0);
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	/* Bottom */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+d);		/*  1 */
	glVertex3f(x+w, y, z+d);	/*  2 */
	glVertex3f(x+w, y, z);		/*  3 */
	glVertex3f(x, y, z);		/*  4 */
	/* Back */
	glVertex3f(x, y+h, z);		/*  5 */
	glVertex3f(x, y+h, z+d);	/*  6 */
	glVertex3f(x, y, z+d);		/*  7 */
	/* Right */
	glVertex3f(x+w, y, z+d);	/*  8 */
	glVertex3f(x+w, y+h, z+d);	/*  9 */
	glVertex3f(x, y+h, z+d);	/* 10 */

	glVertex3f(x, y+h, z);		/* 11 */

	/* Left */
	glVertex3f(x+w, y+h, z);	/* 12 */
	glVertex3f(x+w, y, z);		/* 13 */
	glVertex3f(x+w, y+h, z);	/* 14 */
	/* Front */
	glVertex3f(x+w, y+h, z+d);	/* 15 */
	glEnd();
}

__inline void
draw_textured_node(struct node *n, float w, float h, float d)
{
	float x = n->n_pos.np_x;
	float y = n->n_pos.np_y;
	float z = n->n_pos.np_z;
//	float ux, uy, uz;
	float uw, uh, ud;
	float color[4];
	GLenum param;

	/* Convert to texture units */
#if 0
	/* Too Big */
	ud = d / TEX_SIZE;
	uh = h / TEX_SIZE;
	uw = w / TEX_SIZE;

	/* Too small*/
	ud = d;
	uh = h;
	uw = w;
#endif

	uw = 1.0;
	ud = 2.0;
	uh = 2.0;

//	ux = 0.0;
//	uy = 0.0;
//	uz = 0.0;

	glEnable(GL_TEXTURE_2D);

	/* DEBUG */
	if (st.st_opts & OP_BLEND){
		glEnable(GL_BLEND);

		/* 1 */
		//glBlendFunc(GL_SRC_COLOR, GL_CONSTANT_ALPHA);

		/* 2 Works with: GL_INTENSITY */
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);

		/* 3 Transparent, but alpha value doesn't effect */
		//glBlendFunc(GL_SRC_COLOR, GL_DST_ALPHA);

		/* 4 Works with: GL_INTENSITY */
		//glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

		param = GL_BLEND;
	} else
		param = GL_REPLACE;

	glBindTexture(GL_TEXTURE_2D, jstates[n->n_state].js_fill.f_texid);

	switch (st.st_mode) {
	case SM_JOBS:
		if (n->n_state == JST_USED) {
			color[0] = n->n_job->j_fill.f_r;
			color[1] = n->n_job->j_fill.f_g;
			color[2] = n->n_job->j_fill.f_b;
			color[3] = st.st_alpha_job;
		} else {
			/* Default color, with alpha */
			color[0] = 0.90;
			color[1] = 0.80;
			color[2] = 0.50;
			color[3] = st.st_alpha_oth;
		}
		break;
	case SM_TEMP:
		break;
	case SM_FAIL:
		break;
	}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, param);

	if (st.st_opts & OP_BLEND)
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);


	/* Back  */
	glBegin(GL_POLYGON);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(uh, uw);
	glVertex3f(x+w, y, z);
	glTexCoord2f(uh, 0.0);
	glEnd();


	/* Front */
	glBegin(GL_POLYGON);

#if 0
	Same as Below, execpt using 3f
	glVertex3f(x, y, z+d);
	glTexCoord3f(0.0, 0.0, 0.0);
	glVertex3f(x, y+h, z+d);
	glTexCoord3f(0.0, uw, 0.0);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord3f(uh, uw, 0.0);
	glVertex3f(x+w, y, z+d);
	glTexCoord3f(uh, 0.0, 0.0);
#endif

	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(uh, uw);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(uh, 0.0);
	glEnd();

	/* Right */
	glBegin(GL_POLYGON);
	glVertex3f(x+w, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(0.0, ud);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(uh, ud);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(uh, 0.0);
	glEnd();

	/* Left */
	glBegin(GL_POLYGON);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, ud);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(uh, ud);
	glVertex3f(x, y+h, z);
	glTexCoord2f(uh, 0.0);
	glEnd();

	/* Top */
	glBegin(GL_POLYGON);
	glVertex3f(x, y+h, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(ud, uw);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(ud, 0.0);
	glEnd();

	/* Bottom */
	glBegin(GL_POLYGON);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(ud, uw);
	glVertex3f(x+w, y, z);
	glTexCoord2f(ud, 0.0);
	glEnd();

	/* DEBUG */
	if (st.st_opts & OP_BLEND)
		glDisable(GL_BLEND);

	glDisable(GL_TEXTURE_2D);
}

__inline void
draw_node(struct node *n, float w, float h, float d)
{
	if (st.st_opts & OP_TEX)
		draw_textured_node(n, w, h, d);
	else
		draw_filled_node(n, w, h, d);
	if (st.st_opts & OP_WIRES)
		draw_wireframe_node(n, w, h, d);
}
