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
	if (active_flyby)
		return;
	spkey = glutGetModifiers();
	if (spkey == 0 && selnode != NULL &&
	    button == GLUT_LEFT_BUTTON &&
	    state == GLUT_DOWN)
		panel_toggle(PANEL_NINFO);
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
	rotate_cam(du, dv);
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
