/* $Id$ */

/*
 * Compile using:
 *	gcc -lGL -lglut mon.c
 */

#include <sys/queue.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include <err.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define _PATH_JLIST1 "/home/torque/nids_list_login1"
#define _PATH_JLIST2 "/home/torque/nids_list_login2"

#define WIN_WIDTH		500
#define WIN_HEIGHT		500

#define ST_FREE			1
#define ST_DOWN			2
#define ST_USED			3

#define NROWS			2
#define NCABS			10
#define NCAGES			3
#define NMODS			8
#define NNODES			4

typedef int nodeid_t;
typedef int jobid_t;

struct job {
	jobid_t			j_id;
	SLIST_HEAD(node)	j_nodes;
	SLIST_ENTRY(job)	j_next;
};

struct node {
	nodeid_t		n_id;
	jobid_t			n_jobid;
	int			n_status;
};

SLIST_HEAD(job)			jobs;
struct node			nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
GLfloat 			angle;
	
void
reshape(int width, int height)
{
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-50.0, 50.0, -50.0, 50.0, -150.0, 150.0);
	glMatrixMode(GL_MODELVIEW);
}

void
key(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':
	case 'Q':
		exit(0);
		/* NOTREACHED */
	}
}

void
sp_key(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		angle -= 0.01f;
		orient(angle);
		break;
	case GLUT_KEY_RIGHT:
		angle += 0.01f;
		orient(angle);
		break;
	case GLUT_KEY_UP:
		move(1);
		break;
	case GLUT_KEY_DOWN:
		move(-1);
		break;
	}
}

void
orient(float angle)
{
	lx = sin(angle);
	ly = -cos(angle);
	glLoadIdentity();
	glutLookAt(x, y, z,
	    x + lx, y + ly, z + lz,
	    0.0f, 1.0f, 0.0f);
}

void
move(int dir)
{
	x += dir * lx * 0.1;
	y += dir * lz * 0.1;
	glLoadIdentity();
	glutLookAt(x, y, z,
	    x + lx, y + ly, z + lz,
	    0.0f, 1.0f, 0.0f);
}


void
draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glRotatef(60.0, 0.0, 1.0, 0.0);
	glCallList(BAR);
	glTranslatef(0.0, 30.0, 0.0);
	glRotatef(angle, 1.0, 0.0, 0.0);
	glTranslatef(0.0, -30.0, 0.0);
	glCallList(SWING);
	glFlush();
	glutSwapBuffers();

}

void
make_cluster(void)
{
	int r, cb, cg, m, n, total;

	/* Assign node IDs. */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++, total++)
						nodes[r][cb][cg][m][n].n_id =
						    total;
}

void
make_bar(void)
{
	glNewList(BAR, GL_COMPILE);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 0.0);

	// Bottom
	glVertex3d(-50, 30, -1);
	glVertex3d(50, 30, -1);
	glVertex3d(50, 30, 1);
	glVertex3d(-50, 30, 1);
	// Top
	glVertex3d(-50, 33, -1);
	glVertex3d(50, 33, -1);
	glVertex3d(50, 33, 1);
	glVertex3d(-50, 33, 1);

	glColor3f(1.0, 0.65, 0.0);

	// Front
	glVertex3d(-50, 33, 1);
	glVertex3d(-50, 30, 1);
	glVertex3d(50, 30, 1);
	glVertex3d(50, 33, 1);
	// Back
	glVertex3d(50, 33, -1);
	glVertex3d(50, 30, -1);
	glVertex3d(-50, 30, -1);
	glVertex3d(-50, 33, -1);

	glColor3f(1.0, 0.0, 0.0);

	// Left
	glVertex3d(-50, 33, -1);
	glVertex3d(-50, 30, -1);
	glVertex3d(-50, 30, 1);
	glVertex3d(-50, 33, 1);
	// Right
	glVertex3d(50, 33, 1);
	glVertex3d(50, 30, 1);
	glVertex3d(50, 30, -1);
	glVertex3d(50, 33, -1);
	glEnd();

	glEndList();
}

void
make_swing(void)
{
	glNewList(SWING, GL_COMPILE);

	glColor3f(1.0, 0.0, 1.0);
	glLineWidth(5.0);

	// Draw the ropes
	glBegin(GL_LINES);
	glVertex3d(-20, 30, 0);
	glVertex3d(-20, -30, 0);
	glVertex3d(20, 30, 0);
	glVertex3d(20, -30, 0);
	glEnd();

	glColor3f(0.0, 1.0, 0.0);

	// Draw the sides of the seat
	glBegin(GL_QUAD_STRIP);
	glVertex3d(20, -30, -10);
	glVertex3d(20, -33, -10);
	glVertex3d(-20, -30, -10);
	glVertex3d(-20, -33, -10);
	glVertex3d(-20, -30, 10);
	glVertex3d(-20, -33, 10);
	glVertex3d(20, -30, 10);
	glVertex3d(20, -33, 10);
	glVertex3d(20, -30, -10);
	glVertex3d(20, -33, -10);
	glEnd();

	glColor3f(0.0, 0.0, 1.0);

	// Draw the top and bottom of the seat
	glBegin(GL_QUADS);
	glVertex3d(-20, -33, -10);
	glVertex3d(20, -33, -10);
	glVertex3d(20, -33, 10);
	glVertex3d(-20, -33, 10);

	glVertex3d(-20, -30, -10);
	glVertex3d(20, -30, -10);
	glVertex3d(20, -30, 10);
	glVertex3d(-20, -30, 10);
	glEnd();

	glEndList();
}

void
load_cluster(void)
{
	int i, j, k, l, m;

	for (r = 0; r < NROWS; r++) {
		for (cb = 0; cb < NCABS; cb++) {
			for (cg = 0; cg < NCAGES; cg++) {
				for (:)
			}
		}
	}
}

void
make_cluster(void)
{
	load_cluster();
}


int
main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	if (glutCreateWindow("Monitor") == GL_FALSE)
		errx(1, "CreateWindow");

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);

	make_cluster();

	angle = -39.0;

	/* glutExposeFunc(reshape); */
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutKeyboardFunc(sp_key);
	glutDisplayFunc(draw);
	glutIdleFunc(draw);
	glutMainLoop();
	/* NOTREACHED */
}
