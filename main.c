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

#define _PATH_JOBMAP	"/usr/users/torque/nids_list_login%d"
#define _PATH_PHYSMAP	"/opt/tmp-harness/default/ssconfig/sys%d/nodelist"

#define WIN_WIDTH	500
#define WIN_HEIGHT	500

#define ST_FREE		1
#define ST_DOWN		2
#define ST_USED		3

#define NROWS		2
#define NCABS		10
#define NCAGES		3
#define NMODS		8
#define NNODES		4

typedef int nodeid_t;
typedef int jobid_t;

struct job {
	jobid_t			j_id;
	LIST_HEAD(, node)	j_nodes;
	LIST_ENTRY(job)		j_next;
};

struct node {
	nodeid_t	n_id;
	jobid_t		n_jobid;
	int		n_status;
};

void			orient(float);
void			move(int);

int			logids[] = {0, 8};
LIST_HEAD(, job)	jobs;
struct node		nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
GLfloat 		angle;
float			x, y, z, lx, ly, lz;
GLint			cluster_dl;

#define			NLOGIDS (sizeof(logids) / sizeof(logids[0]))

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
	float lx, ly;

	lx = sin(angle);
	ly = -cos(angle);
	glLoadIdentity();
	gluLookAt(x, y, z,
	    x + lx, y + ly, z + lz,
	    0.0f, 1.0f, 0.0f);
}

void
move(int dir)
{
	x += dir * lx * 0.1;
	y += dir * lz * 0.1;
	glLoadIdentity();
	gluLookAt(x, y, z,
	    x + lx, y + ly, z + lz,
	    0.0f, 1.0f, 0.0f);
}


void
draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glRotatef(angle, 1.0, 0.0, 0.0);
	glTranslatef(0.0, -30.0, 0.0);
	glCallList(cluster_dl);

	glFlush();
	glutSwapBuffers();
}

/*
 *             z
 *            /
 *           /
 *          /
 * (0,0,0) +-------------------------------------------------------- x
 *         |                 (x,y,z+DEPTH)   (x+WIDTH,y,z+DEPTH)
 *         |                         +----------+
 *         |                        /   Top    /|
 *         |                       / |        / |
 *         |              (x,y,z) +----------+  | (x+WIDTH,y,z)
 *         |                      |   (Back) |  |
 *         |                      |  |       |  | Right
 *         |                      |          |  |
 *         |                 Left |  | Front |  |
 *         |                      |          |  |
 *         | (x,y+HEIGHT,z+DEPTH) |  +  -  - |- + (x+WIDTH,y+HEIGHT,z+DEPTH)
 *         |                      | /        | /
 *         |                      | (Bottom) |/
 *         |                      +----------+
 *         |        (x,y+HEIGHT,z) (x+WIDTH,y+HEIGHT,z)
 *         |
 *         y
 */
__inline void
draw_node(int x, int y, int z, int length, int width, int depth)
{
	glColor3f(1.0, 1.0, 0.0);

	/* Bottom */
	glVertex3d(-50, 30, -1);
	glVertex3d(50, 30, -1);
	glVertex3d(50, 30, 1);
	glVertex3d(-50, 30, 1);
	/* Top */
	glVertex3d(-50, 33, -1);
	glVertex3d(50, 33, -1);
	glVertex3d(50, 33, 1);
	glVertex3d(-50, 33, 1);

	glColor3f(1.0, 0.65, 0.0);

	/* Front */
	glVertex3d(-50, 33, 1);
	glVertex3d(-50, 30, 1);
	glVertex3d(50, 30, 1);
	glVertex3d(50, 33, 1);
	/* Back */
	glVertex3d(50, 33, -1);
	glVertex3d(50, 30, -1);
	glVertex3d(-50, 30, -1);
	glVertex3d(-50, 33, -1);

	glColor3f(1.0, 0.0, 0.0);

	/* Left */
	glVertex3d(-50, 33, -1);
	glVertex3d(-50, 30, -1);
	glVertex3d(-50, 30, 1);
	glVertex3d(-50, 33, 1);
	/* Right */
	glVertex3d(50, 33, 1);
	glVertex3d(50, 30, 1);
	glVertex3d(50, 30, -1);
	glVertex3d(50, 33, -1);
}

void
make_cluster(void)
{
	int r, cb, cg, m, n;
	int rowdepth, rowspace;
	int cabwidth, cabspace;
	int cageheight, cagespace;
	int modwidth, modspace;
	int nodewidth, nodeheight, nodedepth;

	rowspace = 50;
	cabspace = 25;
	cagespace = 10;
	modspace = 5;

	nodewidth = nodeheight = nodedepth = 10;
	modwidth = nodewidth;
	cageheight = nodeheight * 2;
	cabwidth = modwidth * NMODS;
	rowdepth = nodedepth * 2;

	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
	glBegin(GL_QUADS);
	for (r = 0; r < NROWS; r++, z += rowdepth + rowspace) {
		for (cb = 0; cb < NCABS; cb++, x += cabwidth + cabspace) {
			for (cg = 0; cg < NCAGES; cg++, y += cageheight + cagespace) {
				for (m = 0; m < NMODS; m++, x += modwidth + modspace) {
					for (n = 0; n < NNODES; n++) {
						draw_node(x,
						    y + nodeheight * (n % (NNODES/2)),
						    z + nodedepth * (n / (NNODES/2)),
						    nodewidth,
						    nodeheight,
						    nodedepth);
					}
				}
				x -= (modwidth + modspace) * NMODS;
			}
			y -= (cageheight + cagespace) * NCAGES;
		}
		x -= (cabwidth + cabspace) * NCABS;
	}
	glEnd();
	glEndList();
}

int
main(int argc, char *argv[])
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

	parse_physmap();
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
