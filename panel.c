/* $Id$ */

#include <sys/types.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <stdio.h>
#include <string.h>

#include "mon.h"

#define FPS_STRLEN	10
#define TEXT_HEIGHT	25
#define X_SPEED		2
#define Y_SPEED		1

#define MAX_INFO	50
#define INFO_ITEMS	4

void
draw_fps(void)
{
	static double sx = 0;
	char frate[FPS_STRLEN];
	double x, y, w, h;
	double cx, cy;
	int vp[4];
	size_t i;

	/* Save our state and set things up for 2d. */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glGetIntegerv(GL_VIEWPORT, vp);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, vp[2], 0.0, vp[3]);

	/* Create string */
	memset(frate, 0, FPS_STRLEN);
	snprintf(frate, sizeof(frate), "FPS: %ld", fps);

	/* Coordinates */
	w = sizeof(frate) * 8;
	h = TEXT_HEIGHT;
	x = vp[2] - w;
	y = vp[3];

	/*
	 * Adjust current position.
	 * If on, move onto screen.  If off, move off screen.
	 */
	if (sx == 0)
		sx = vp[2];
	else if (sx > x - 1 && (st.st_panels & PANEL_FPS))
		sx -= X_SPEED;
	else if (sx < vp[2]+1 && (st.st_panels & PANEL_FPS) == 0)
		sx += X_SPEED;
	else
		active_fps = 0;

	/* Draw the frame rate */
	glColor4f(1.0,1.0,1.0,1.0);

	/* Account for text height and fps digit length. */
	cx = (fps > 999 ? 0 : 8);
	cy = 8;

	glRasterPos2d(sx+cx,y-TEXT_HEIGHT+cy);

	for (i = 0; i < sizeof(frate); i++)
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, frate[i]);

	/* Draw polygon/wireframe around fps. */
	for (i = 0; i < 2; i++){
		if (i == 0){
			glBegin(GL_POLYGON);
			glColor4f(0.20, 0.40, 0.5, 1.0);
		} else {
			glLineWidth(2.0);
			glBegin(GL_LINE_LOOP);
			glColor4f(0.40, 0.80, 1.0, 1.0);
		}

		glVertex2d(sx, y);
		glVertex2d(sx+w, y);
		glVertex2d(sx+w, y-h);
		glVertex2d(sx, y-h);

		glEnd();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

void
draw_node_info(void)
{
	static size_t len = 0;
	static double sx = 0;
	static double sy = 0;
	double x, y, w, h, cy;
	int i, vp[4];
	size_t j;
	char str[INFO_ITEMS][MAX_INFO] = {
		"Owner: %s",
		"Job Name: %s",
		"Duration: %d",
		"Number CPU's: %d"
	};

	/* TODO: snprintf any data into our above strings */

	/* Calculate the max string length */
	if (len == 0)
		for (i = 0, j = 0; i < INFO_ITEMS; i++) {
			len = (j > strlen(str[i]) ? j : strlen(str[i]));
			j = len;
		}

	/* Save our state and set things up for 2d */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glGetIntegerv(GL_VIEWPORT, vp);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, vp[2], 0.0, vp[3]);

	/* Take into account draw_fps (1 row). */
	w = len * 8;
	h = INFO_ITEMS * TEXT_HEIGHT;
	x = vp[2] - w;
	y = vp[3] - (st.st_panels & PANEL_FPS ? 1.25 : 0.25) * TEXT_HEIGHT;

	/*
	 * Adjust current position.
	 * If on, move onto screen.  If off, move off screen.
	 */
	if (sx == 0)
		sx = vp[2];
	else if (sx > x - 1 && (st.st_panels & PANEL_NINFO))
		sx -= X_SPEED;
	else if (sx < vp[2]+1 && (st.st_panels & PANEL_NINFO) == 0)
		sx += X_SPEED;
	else
		active_ninfo = 0;

	/*
	 * Autoslide up or down if the fps menu is enabled.
	 * If on, go up.  If off, slide down.
	 */
	if (active_fps) {
		if (sy == 0)
			sy = y;
		else if (sy > y && (st.st_panels & PANEL_FPS))
			sy -= Y_SPEED;
		else if (sy < y && (st.st_panels & PANEL_FPS) == 0)
			sy += Y_SPEED;
		else
			active_ninfo = 0;
		y = sy;
	}

	/* Factor to center text on y axis */
	cy = 8;

	/* Draw node info */
	glColor4f(1.0,1.0,1.0,1.0);
	for (i = 0; i < INFO_ITEMS; i++) {
		/* Set the position of the text (account for fps!) */
		glRasterPos2d(sx, (y-(i+1)*TEXT_HEIGHT)+cy);

		for (j = 0; j < sizeof(str[i]); j++)
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, str[i][j]);
	}

	/* Draw a polygon around node legend */
	for (i = 0; i < 2; i++) {
		if (i == 0) {
			glBegin(GL_POLYGON);
			glColor4f(0.20, 0.40, 0.5, 1.0);
		} else {
			glLineWidth(2.0);
			glBegin(GL_LINE_LOOP);
			glColor4f(0.40, 0.80, 1.0, 1.0);
		}

		/* Draw the polygon around the framerate */
		glVertex2d(sx, y);
		glVertex2d(sx+w, y);
		glVertex2d(sx+w, y-h);
		glVertex2d(sx, y-h);

		glEnd();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}
