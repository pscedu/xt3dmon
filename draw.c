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

#define TWEEN_MAX_POS	(2.0f)
#define TWEEN_MAX_LOOK	(0.01f)
#define TWEEN_THRES	(0.001f)
#define TWEEN_AMT	(0.05f)

__inline int
tween(__unused float start, float *cur, float stop, float max)
{
	float amt;

	if (stop != *cur) {
		amt = (stop - *cur) * TWEEN_AMT;
		amt = MIN(amt, max);
		*cur += amt;
		if (fabs(stop - *cur) < TWEEN_THRES)
			*cur = stop;
		return (1);
	}
	return (0);
}

struct vec logv;

void
draw(void)
{
	update_flyby();

	if (st.st_vmode == VM_LOGICAL)
		if (fabs(logv.v_x - st.st_x) > LOGWIDTH / 2 ||
		    fabs(logv.v_y - st.st_y) > LOGHEIGHT / 2 ||
		    fabs(logv.v_z - st.st_z) > LOGDEPTH / 2) {
		    	logv.v_x = st.st_x;
		    	logv.v_y = st.st_y;
		    	logv.v_z = st.st_z;
			rebuild(RO_COMPILE);
		}

	if (st.st_opts & OP_TWEEN)
		if (tween(ox, &st.st_x, tx, TWEEN_MAX_POS) |
		    tween(oy, &st.st_y, ty, TWEEN_MAX_POS) |
		    tween(oz, &st.st_z, tz, TWEEN_MAX_POS) |
		    tween(olx, &st.st_lx, tlx, TWEEN_MAX_LOOK) |
		    tween(oly, &st.st_ly, tly, TWEEN_MAX_LOOK) |
		    tween(olz, &st.st_lz, tlz, TWEEN_MAX_LOOK))
			cam_update();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (st.st_opts & OP_GROUND)
		glCallList(ground_dl);

	glCallList(cluster_dl);
	draw_panels();

	glClearColor(0.0, 0.0, 0.0, 1.0);
	if (st.st_opts & OP_CAPTURE || gDebugCapture)
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
draw_filled_node(struct node *n)
{
	float x = n->n_v->v_x;
	float y = n->n_v->v_y;
	float z = n->n_v->v_z;
	float w = NODEWIDTH;
	float h = NODEHEIGHT;
	float d = NODEDEPTH;
	float r, g, b, a;

	r = n->n_fillp->f_r;
	g = n->n_fillp->f_g;
	b = n->n_fillp->f_b;
	a = n->n_fillp->f_a;

	if (st.st_opts & OP_BLEND) {
		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
	} else
		a = 1.0;

#if 0
	if(st.st_opts & OP_DEBUG) {
//		GLfloat mat_specular[] =	{0.3, 0.8, 0.8, 0.45};
		GLfloat mat_specular[] =	{1.0, 1.0, 1.0, 1.0};

		/* 0 - 128 */
		GLfloat mat_shininess[] =	{100.0};
//		GLfloat white_light[] =		{1.0, 1.0, 1.0, 1.0};
//		GLfloat spec_light[] =		{1.0, 1.0, 1.0, 1.0};
		GLfloat white_light[] =		{r, g, b, 1.0};
		GLfloat spec_light[] =		{r, g, b, 1.0};
		GLfloat lmodel_ambient[] = 	{0.1, 0.1, 0.1, 0.0};

		/* (x,y,z,w) if w != 0 positional instead of directional */
		GLfloat light_pos[] =		{110.74, 9.40, 0.61, 1.0};
		GLfloat spot_dir[] = 		{0.26, 0.0, 0.96};

		/* Follow Pos */
//		GLfloat light_pos[] =		{st.st_x, st.st_y, st.st_z, 1.0};
//		GLfloat spot_dir[] = 		{st.st_lx, st.st_ly, st.st_lz};

		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec_light);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
//		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

		glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 15.0);
		glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_dir);
		glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 8.0);

		glEnable(GL_NORMALIZE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
	}
#endif

	glColor4f(r, g, b, a);

	glBegin(GL_QUADS);

	/* Back  */
	glVertex3f(x, y, z);
	glVertex3f(x, y+h, z);
	glVertex3f(x+w, y+h, z);
	glVertex3f(x+w, y, z);

	/* Front */
	glVertex3f(x, y, z+d);
	glVertex3f(x, y+h, z+d);
	glVertex3f(x+w, y+h, z+d);
	glVertex3f(x+w, y, z+d);

	/* Right */
	glVertex3f(x+w, y, z);
	glVertex3f(x+w, y, z+d);
	glVertex3f(x+w, y+h, z+d);
	glVertex3f(x+w, y+h, z);

	/* Left */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+d);
	glVertex3f(x, y+h, z+d);
	glVertex3f(x, y+h, z);

	/* Top */
	glVertex3f(x, y+h, z);
	glVertex3f(x, y+h, z+d);
	glVertex3f(x+w, y+h, z+d);
	glVertex3f(x+w, y+h, z);

	/* Bottom */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+d);
	glVertex3f(x+w, y, z+d);
	glVertex3f(x+w, y, z);

	glEnd();

#if 0
	if (st.st_opts & OP_BLEND)
		glDisable(GL_BLEND);

	if(st.st_opts & OP_DEBUG) {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
	}
#endif

}

__inline void
draw_wireframe_node(struct node *n)
{
	float x = n->n_v->v_x;
	float y = n->n_v->v_y;
	float z = n->n_v->v_z;
	float w = NODEWIDTH;
	float h = NODEHEIGHT;
	float d = NODEDEPTH;

	/* Wireframe outline */
	x -= WFRAMEWIDTH;
	y -= WFRAMEWIDTH;
	z -= WFRAMEWIDTH;
	w += 2.0f * WFRAMEWIDTH;
	h += 2.0f * WFRAMEWIDTH;
	d += 2.0f * WFRAMEWIDTH;

	/* Antialiasing */
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

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

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

__inline void
draw_textured_node(struct node *n)
{
	float x = n->n_v->v_x;
	float y = n->n_v->v_y;
	float z = n->n_v->v_z;
	float w = NODEWIDTH;
	float h = NODEHEIGHT;
	float d = NODEDEPTH;
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
	color[3] = n->n_fillp->f_a;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, param);

	if (st.st_opts & OP_BLEND)
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

	/* Polygon Offset */
	if(st.st_opts & OP_POLYOFFSET) {
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 3.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

#if 0
	if(st.st_opts & OP_DEBUG) {
		GLfloat mat_specular[] =	{0.3, 0.8, 0.8, 0.45};
//		GLfloat mat_specular[] =	{1.0, 1.0, 1.0, 1.0};

		/* 0 - 128 */
		GLfloat mat_shininess[] =	{10.0};
		GLfloat white_light[] =		{1.0, 1.0, 1.0, 1.0};
		GLfloat spec_light[] =		{1.0, 1.0, 1.0, 1.0};
		GLfloat lmodel_ambient[] = 	{0.6, 0.6, 0.6, 1.0};

		/* (x,y,z,w) if w != 0 positional instead of directional */
		GLfloat light_pos[] =		{1.0, 1.0, 1.0, 0.0};
		GLfloat spot_dir[] = 		{-1.0, -1.0, 0.0};

		glClearColor(0.0,0.0,0.0,0.0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec_light);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
//		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

		glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0);
		glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_dir);
		glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 2.0);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_NORMALIZE);
	}
#endif

	glBegin(GL_QUADS);

	/* Back  */
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(uh, uw);
	glVertex3f(x+w, y, z);
	glTexCoord2f(uh, 0.0);

	/* Front */
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(uh, uw);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(uh, 0.0);

	/* Right */
	glVertex3f(x+w, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(0.0, ud);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(uh, ud);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(uh, 0.0);

	/* Left */
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, ud);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(uh, ud);
	glVertex3f(x, y+h, z);
	glTexCoord2f(uh, 0.0);

	/* Top */
	glVertex3f(x, y+h, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(ud, uw);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(ud, 0.0);

	/* Bottom */
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(ud, uw);
	glVertex3f(x+w, y, z);
	glTexCoord2f(ud, 0.0);

	glEnd();

	if (st.st_opts & OP_BLEND)
		glDisable(GL_BLEND);

	/* Disable Polygon Offset */
	if(st.st_opts & OP_POLYOFFSET) {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

#if 0
	if(st.st_opts & OP_DEBUG) {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_NORMALIZE);
	}
#endif

	glDisable(GL_TEXTURE_2D);
}

__inline void
draw_node_label(struct node *n)
{
}

__inline void
draw_node(struct node *n)
{
	if (st.st_opts & OP_TEX)
		draw_textured_node(n);
	else
		draw_filled_node(n);
	if (st.st_opts & OP_WIRES)
		draw_wireframe_node(n);
	if (st.st_opts & OP_NLABELS)
		draw_node_label(n);
}

void
make_ground(void)
{
	if (ground_dl)
		glDeleteLists(ground_dl, 1);
	ground_dl = glGenLists(1);
	glNewList(ground_dl, GL_COMPILE);

	glBegin(GL_QUADS);

	/* Ground */
	glColor3f(0.4f, 0.4f, 0.4f);
	glVertex3f( -5.0f, 0.0f, -5.0f);
	glVertex3f( -5.0f, 0.0f, 20.0f);
	glVertex3f(310.0f, 0.0f, 20.0f);
	glVertex3f(310.0f, 0.0f, -5.0f);

	/* x-axis */
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-200.0f, -0.1f);
	glVertex2f(-200.0f,  0.1f);
	glVertex2f( 200.0f,  0.1f);
	glVertex2f( 200.0f, -0.1f);

	/* y-axis */
	glColor3f(0.6f, 0.6f, 1.0f);
	glVertex2f(-0.1f, -200.0f);
	glVertex2f(-0.1f,  200.0f);
	glVertex2f( 0.1f,  200.0f);
	glVertex2f( 0.1f, -200.0f);

	/* z-axis */
	glColor3f(1.0f, 0.9f, 0.0f);
	glVertex3f(-0.1f, -0.1f, -200.0f);
	glVertex3f(-0.1f,  0.1f, -200.0f);
	glVertex3f(-0.1f,  0.1f,  200.0f);
	glVertex3f(-0.1f, -0.1f,  200.0f);
	glVertex3f( 0.1f, -0.1f, -200.0f);
	glVertex3f( 0.1f,  0.1f, -200.0f);
	glVertex3f( 0.1f,  0.1f,  200.0f);
	glVertex3f( 0.1f, -0.1f,  200.0f);

	glEnd();
	glEndList();
}
