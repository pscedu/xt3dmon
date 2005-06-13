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

#include "cdefs.h"
#include "mon.h"

#define TWEEN_MAX	(2.0f)
#define TWEEN_THRES	(0.001f)
#define TWEEN_AMT	(0.025f)

__inline int
tween(float start, float *cur, float stop)
{
	float amt;

	if (stop != *cur) {
		amt = (stop - *cur) * TWEEN_AMT;
		amt = MIN(amt, TWEEN_MAX);
		*cur += amt;
		if (fabs(stop - *cur) < TWEEN_THRES)
			*cur = stop;
		return (1);
	}
	return (0);

#if 0
	if (stop - *cur) {
//printf("[%.3f] ", stop - *cur);
//printf("[frac: %.3f] ", (*cur - start) / (stop - start));
//		*cur += (stop - start) * TWEEN_AMT;
//printf("[%.3f, o %.3f ", *cur, (stop - start) *
// expf(-powf((*cur - start) * 3.0f / (stop - start), 2.0f) / 2.0f) / (2.0f * PI));
		float x = (*cur - start) * 6.0f / (stop - start) - 3.0f;
		*cur = start + (stop - start) * expf(-powf(x, 2.0f) / 2.0f) / (2.0f * PI);
//printf("%.3f] ", *cur);
		if (fabs(stop - *cur) < TWEEN_THRES)
			*cur = stop;
		return (1);
	}
	return (0);
#endif
}

void
draw(void)
{
#if 0
	/* record user commands */
	if(build_flyby)
		write_flyby();

	if (active_flyby) {
		read_flyby();
//		make_cluster();
	}
#endif
	update_flyby();

//printf("."); fflush(stdout);
	if (st.st_opts & OP_TWEEN)
		if (tween(ox, &st.st_x, tx) |
		    tween(oy, &st.st_y, ty) |
		    tween(oz, &st.st_z, tz) |
		    tween(olx, &st.st_lx, tlx) |
		    tween(oly, &st.st_ly, tly) |
		    tween(olz, &st.st_lz, tlz))
{			adjcam();
//printf("#");
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

	r = n->n_fillp->f_r;
	g = n->n_fillp->f_g;
	b = n->n_fillp->f_b;
//	a = n->n_fillp->f_a;

	/* Set alpha depending on usage */
	if(n->n_state == JST_USED)
		a = st.st_alpha_job;
	else
		a = st.st_alpha_oth;

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

	/*
	** XXX - readd antialiasing
	**     - fix linewidth changes when turning on blend mode
	*/

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
	float uw, uh, ud;
	float color[4];
	GLenum param;

	/* Convert to texture units */
	uw = 1.0;
	ud = 2.0;
	uh = 2.0;

	glEnable(GL_TEXTURE_2D);

	if (st.st_opts & OP_BLEND){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		param = GL_BLEND;
	} else
		param = GL_REPLACE;

	glBindTexture(GL_TEXTURE_2D, jstates[n->n_state].js_fill.f_texid);

	color[0] = n->n_fillp->f_r;
	color[1] = n->n_fillp->f_g;
	color[2] = n->n_fillp->f_b;
//	color[3] = n->n_fillp->f_a;

	/* Set alpha depending on usage */
	if(n->n_state == JST_USED)
		color[3] = st.st_alpha_job;
	else
		color[3] = st.st_alpha_oth;

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
