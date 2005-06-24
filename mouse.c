/* $Id$ */

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <math.h>

#include "cdefs.h"
#include "mon.h"

int	 spkey, lastu, lastv;
GLuint	 selbuf[100];
int	 render_mode = RM_RENDER;

void
mouseh(__unused int button, __unused int state, int u, int v)
{
	if (active_flyby) /* XXX */
		return;
	spkey = glutGetModifiers();
	if (spkey == 0 &&
	    button == GLUT_LEFT_BUTTON &&
	    state == GLUT_DOWN)
		render_mode = RM_SELECT;
	lastu = u;
	lastv = v;
}

void
m_activeh_default(int u, int v)
{
	int du = u - lastu, dv = v - lastv;

	if (abs(du) + abs(dv) <= 1)
		return;
	lastu = u;
	lastv = v;

	tween_pushpop(TWF_PUSH | TWF_LOOK | TWF_POS);
	if (du != 0 && spkey & GLUT_ACTIVE_CTRL)
		cam_revolve(du);
	if (dv != 0 && spkey & GLUT_ACTIVE_SHIFT)
		cam_rotatev(dv);
	tween_pushpop(TWF_POP | TWF_LOOK | TWF_POS);
}

void
m_activeh_free(int u, int v)
{
	int du = u - lastu, dv = v - lastv;

	if (abs(du) + abs(dv) <= 1)
		return;
	lastu = u;
	lastv = v;

	tween_pushpop(TWF_PUSH | TWF_LOOK);
	if (du != 0 && spkey == GLUT_ACTIVE_CTRL)
		cam_rotateu(du);
	if (dv != 0 && spkey == GLUT_ACTIVE_CTRL)
		cam_rotatev(dv);
	tween_pushpop(TWF_POP | TWF_LOOK);
}

void
m_passiveh_default(int u, int v)
{
	lastu = u;
	lastv = v;

//	detect_node(u, v);
//	render_mode = RM_SELECT;
}

void
m_passiveh_null(int u, int v)
{
	lastu = u;
	lastv = v;
}

void
m_activeh_null(int u, int v)
{
	lastu = u;
	lastv = v;
}

void
sel_record_begin(void)
{
	GLint viewport[4];

	glSelectBuffer(sizeof(selbuf)/sizeof(selbuf[0]), selbuf);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glRenderMode(GL_SELECT);
	glInitNames();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPickMatrix(lastu, win_height - lastv, 5, 5, viewport);
	gluPerspective(FOVY, ASPECT, 0.1, WI_CLIP);
	glMatrixMode(GL_MODELVIEW);
}

void
sel_record_process(GLint nrecs)
{
	struct node *newsn, *n;
	float diff, mindiff;
	GLuint *p, len, j;
	struct vec v;
	int i;

	newsn = NULL;
	mindiff = FLT_MAX;
	/* XXX:  sanity-check nrecs? */
	for (i = 0, p = (GLuint *)selbuf; i < nrecs; i++) {
		/*
		 * The record consists of the following:
		 *	- the number of names in this stack frame,
		 *	- the minimum and maximum depths of all
		 *	  vertices hit since the previous event, and
		 *	- stack contents, bottom first
		 */
		len = *p;

		/* Skip min/max depths. */
		p += 3;
		for (j = 0; j < len; j++, p++) {
			/* Find closest node. */
			if ((n = node_for_nid(*p)) == NULL)
				continue;
			v.v_w = st.st_x - n->n_v->v_x;
			v.v_h = st.st_y - n->n_v->v_y;
			v.v_d = st.st_z - n->n_v->v_z;
			diff = fabs(v.v_w) + fabs(v.v_h) + fabs(v.v_d);
			if (diff < mindiff) {
				newsn = n;
				mindiff = diff;
			}
		}
	}
	if (newsn != NULL)
		select_node(newsn);
}

void
sel_record_end(void)
{
	int nrecs;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glFlush();

	if ((nrecs = glRenderMode(GL_RENDER)) != 0)
		sel_record_process(nrecs);
	render_mode = RM_RENDER;

	rebuild(RF_PERSPECTIVE);
}
