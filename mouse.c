/* $Id$ */

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <limits.h>

#include "cdefs.h"
#include "mon.h"

int	 spkey, lastu, lastv;
GLuint	 selbuf[1000];
int	 render_mode = RM_RENDER;

void
mouseh_null(__unused int button, __unused int state, __unused int u, __unused int v)
{
//	spkey = glutGetModifiers();
}

void
mouseh_default(__unused int button, __unused int state, int u, int v)
{
	spkey = glutGetModifiers();
//	if (spkey == 0 &&
//	    button == GLUT_LEFT_BUTTON &&
//	    state == GLUT_DOWN)
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

	tween_push(TWF_LOOK | TWF_POS);
	if (du != 0 && spkey & GLUT_ACTIVE_CTRL)
		cam_revolve(du);
	if (spkey & GLUT_ACTIVE_SHIFT) {
		if (dv != 0)
			cam_rotatev(dv);
		if (du != 0)
			cam_rotateu(du);
	}
	tween_pop(TWF_LOOK | TWF_POS);
}

void
m_passiveh_default(int u, int v)
{
	lastu = u;
	lastv = v;

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
	float clip;

	clip = MIN(WI_CLIPX, WI_CLIPY);
	clip = MIN(clip, WI_CLIPZ);

	glSelectBuffer(sizeof(selbuf) / sizeof(selbuf[0]), selbuf);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glRenderMode(GL_SELECT);
	glInitNames();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPickMatrix(lastu, win_height - lastv, 5, 5, viewport);
	gluPerspective(FOVY, ASPECT, 0.1, clip);
	glMatrixMode(GL_MODELVIEW);
}

#define SBI_LEN		0
#define SBI_UDST	1
#define SBI_VDST	2
#define SBI_NAMEOFF	3

void
sel_record_process(GLint nrecs)
{
	GLuint minu, minv, *p, nid;
	struct node *n;
	int i, found;

	nid = 0; /* gcc */
	found = 0;
	minu = minv = UINT_MAX;
	/* XXX:  sanity-check nrecs? */
	for (i = 0, p = (GLuint *)selbuf; i < nrecs; i++, p += 3 + p[SBI_LEN]) {
		/*
		 * Each record consists of the following:
		 *	- the number of names in this stack frame,
		 *	- the minimum and maximum depths of all
		 *	  vertices hit since the previous event, and
		 *	- stack contents, bottom first
		 */
		if (p[SBI_LEN] != 1)
			continue;
		if (p[SBI_UDST] <= minu) {
			minu = p[SBI_UDST];
			if (p[SBI_VDST] < minv) {
				minv = p[SBI_VDST];
				found = 1;
				nid = p[SBI_NAMEOFF];
			}
		}

	}
	if (found && (n = node_for_nid(nid)) != NULL) {
		switch (spkey) {
		case GLUT_ACTIVE_SHIFT:
			sel_add(n);
			break;
		case GLUT_ACTIVE_CTRL:
			sel_del(n);
			break;
		case 0:
			sel_set(n);
			break;
		}
		if (SLIST_EMPTY(&selnodes))
			panel_hide(PANEL_NINFO);
		else
			panel_show(PANEL_NINFO);
	}
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
