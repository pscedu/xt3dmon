/* $Id$ */

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include "cdefs.h"
#include "mon.h"

int	 spkey, lastu, lastv;

void
mouseh(__unused int button, __unused int state, int u, int v)
{
	struct node *oldsn;

	if (active_flyby) /* XXX */
		return;
	spkey = glutGetModifiers();
	if (spkey == 0 &&
	    button == GLUT_LEFT_BUTTON &&
	    state == GLUT_DOWN) {
	    	oldsn = selnode;
		if (selnode) {
			selnode->n_fillp = selnode->n_ofillp;
			selnode = NULL;
		}
		detect_node(u, v);
		if (selnode == NULL)
			panel_hide(PANEL_NINFO);
		else
			panel_show(PANEL_NINFO);
		if (oldsn != selnode)
			rebuild(RF_CLUSTER);
	}
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

	detect_node(u, v);
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
