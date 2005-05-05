/* $Id$ */

/*
 * Compile using:
 *	gcc -lGL -lglut mon.c
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mon.h"

#define WIN_WIDTH	800
#define WIN_HEIGHT	600

#define XORIGIN		((cabwidth * NCABS + cabspace * (NCABS - 1)) / 2)
#define YORIGIN		((cageheight * NCAGES + cagespace * (NCAGES - 1)) / 2)
#define	ZORIGIN		((rowdepth * NROWS + rowspace * (NROWS - 1)) / 2)

#define SLEEP_INTV	5

#define DIR_X 0
#define DIR_Y 1
#define DIR_Z 2

void			 adjcam(void);
void			 parse_jobmap(void);
void			 parse_physmap(void);

struct job		**jobs;
size_t			 njobs;
struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
GLfloat 		 anglex = -5.0f, angley = 0.1f;
float			 x = -15.0f, y = 9.0f, z = 15.0f;
float			 lx = 0.9f, ly = 0.0f, lz = -0.3f;
int			 spkey, xpos, ypos;
GLint			 cluster_dl;
struct node		*invmap[NROWS * NCABS * NCAGES * NMODS * NNODES];
struct timeval		 lastsync;
float			 rowdepth, rowspace;
float			 cabwidth, cabspace;
float			 cageheight, cagespace;
float			 modwidth, modheight, moddepth, modspace;
float			 nodewidth, nodeheight, nodedepth, nodespace;

struct state states[] = {
	{ "Free",		1.0, 1.0, 1.0 },
	{ "Disabled (PBS)",	1.0, 0.0, 0.0 },
	{ "Disabled (HW)",	0.66, 0.66, 0.66 },
	{ NULL,			0.0, 0.0, 0.0 },
	{ "I/O",		1.0, 1.0, 0.0 },
	{ "Unaccounted",	0.0, 0.0, 1.0 },
	{ "Bad",		1.0, 0.75, 0.75 },
	{ "Checking",		0.0, 1.0, 0.0 }
};

__inline void
adjcam(void)
{
//printf("(%f, %f, %f) -> (%f, %f, %f)\n", x, y, z, lx, ly, lz);
	glLoadIdentity();
	gluLookAt(x, y, z,
	    x + lx, y + ly, z + lz,
	    0.0f, 1.0f, 0.0f);
}

void
reshape(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, w, h);
	gluPerspective(45, 1.0f * w / h, 1, 1000);
	glMatrixMode(GL_MODELVIEW);
	adjcam();
//	glOrtho(-50.0, 50.0, -50.0, 50.0, -150.0, 150.0);
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

/*
 *	       z
 *	      /
 *	     / PageUp /\
 *	    /
 *	   /
 *	  / PageDown \/
 *	 /
 *	+------------------------------------------ x
 *	|	Left <-		-> Right
 *	|
 *	|
 *	| Up /\
 *	|
 *	|
 *	| Down \/
 *	|
 *	|
 *	y
 */
void
sp_key(int key, int u, int v)
{
	switch (key) {
	case GLUT_KEY_LEFT:
printf("lz: %f [%f]\n", lz, lz * 0.3);
		x += lz * 0.3;
		z -= lx * 0.3;
		break;
	case GLUT_KEY_RIGHT:
		x -= lz * 0.3;
		z += lx * 0.3;
		break;
	case GLUT_KEY_UP:
		x += lx * 0.3;
		z += lz * 0.3;
		break;
	case GLUT_KEY_DOWN:
		x -= lx * 0.3;
		z -= lz * 0.3;
		break;
	case GLUT_KEY_PAGE_UP:
		y += 1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		y -= 1;
		break;
	default:
		return;
	}
	adjcam();
}

void
mouse(int button, int state, int u, int v)
{
	spkey = glutGetModifiers();
	xpos = u;
	ypos = v;
}

void
active_m(int u, int v)
{
	int du = u - xpos, dv = v - ypos;
	float t, r;

	xpos = u;
	ypos = v;
	if (spkey != GLUT_ACTIVE_CTRL)
		return;
	if (du != 0) {
		r = sqrt((x - XORIGIN) * (x - XORIGIN) +
		    (z - ZORIGIN) * (z - ZORIGIN));

		t = acosf((x - XORIGIN) / r);

		if (x > XORIGIN)
			t += PI;
	//	else if (x < XORIGIN && z < ZORIGIN)
	//		t += 3 * PI / 2;

printf("r: %.2f [%.2f -> ", r, t);
		t += .05 * (float)du;
printf("%.2f] (%.2f,%.2f) -> ", t, x, z);
		x = r * cos(t) + XORIGIN;
		z = r * sin(t) + ZORIGIN;
		lx = (XORIGIN - x) / r;
		lz = (ZORIGIN - z) / r;
printf("(%.2f,%.2f) [du: %d, dv: %d] c(%.2f,%.2f) [%.2f,%.2f]\n",
    x, z, du, dv, XORIGIN, ZORIGIN, lx, lz);
/*
		anglex += (du < 0) ? 0.0025f : -0.0025f;
		lx = sin(anglex);
		lz = -cos(anglex);
*/
	}
	if (dv != 0) {
/*
		angley += (dv < 0) ? 0.0025f : -0.0025f;
		ly = sin(angley);
		lz = -cos(angley);
*/
	}
	adjcam();
}

void
passive_m(int u, int v)
{
	xpos = u;
	ypos = v;
}

void
draw(void)
{
	struct timeval tv;
static int cnt = 0;
cnt++;

/*
	if (gettimeofday(&tv, NULL) == -1)
		err(1, "gettimeofday");
	if (lastsync.tv_sec + SLEEP_INTV < tv.tv_sec) {
printf("fps: %d\n", cnt / SLEEP_INTV);
cnt = 0;
		lastsync.tv_sec = tv.tv_sec;
	}
*/


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Ground */
	glColor3f(0.4f, 0.4f, 0.4f);
	glBegin(GL_QUADS);
	glVertex3f( -5.0f, 0.0f, -5.0f);
	glVertex3f( -5.0f, 0.0f, 25.0f);
	glVertex3f(250.0f, 0.0f, 25.0f);
	glVertex3f(250.0f, 0.0f, -5.0f);
	glEnd();

	/* x-axis */
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glVertex3f(-200.0f, -0.1f, -0.1f);
	glVertex3f(-200.0f, -0.1f,  0.1f);
	glVertex3f( 200.0f, -0.1f,  0.1f);
	glVertex3f( 200.0f, -0.1f, -0.1f);

	glVertex3f( 200.0f,  0.1f, -0.1f);
	glVertex3f( 200.0f,  0.1f,  0.1f);
	glVertex3f(-200.0f,  0.1f,  0.1f);
	glVertex3f(-200.0f,  0.1f, -0.1f);
	glEnd();

	/* y-axis */
	glColor3f(0.6f, 0.6f, 1.0f);
	glBegin(GL_QUADS);
	glVertex3f(-0.1f, -200.0f, -0.1f);
	glVertex3f(-0.1f, -200.0f,  0.1f);
	glVertex3f(-0.1f,  200.0f,  0.1f);
	glVertex3f(-0.1f,  200.0f, -0.1f);

	glVertex3f( 0.1f, -200.0f, -0.1f);
	glVertex3f( 0.1f, -200.0f,  0.1f);
	glVertex3f( 0.1f,  200.0f,  0.1f);
	glVertex3f( 0.1f,  200.0f, -0.1f);
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

/*
	glLoadIdentity();
	glRotatef(anglex, 1.0, 0.0, 0.0);

	glTranslatef(0.0, -30.0, 0.0);

	glFlush();
*/
	glCallList(cluster_dl);

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
draw_node(struct node *n, float x, float y, float z, float width,
    float height, float depth)
{
	float r, g, b;

	if (n->n_state == ST_USED)
		r = states[n->n_state].st_r;
	else {
		r = states[n->n_state].st_r;
		g = states[n->n_state].st_g;
		b = states[n->n_state].st_b;
	}
	glColor3f(r, g, b);

	/* Bottom */
	glVertex3f(x, y + height, z);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
	/* Top */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y, z);

	/* Front */
	glVertex3f(x, y + height, z);
	glVertex3f(x, y, z);
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y + height, z);
	/* Back */
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x, y + height, z + depth);

	/* Left */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x, y + height, z);
	/* Right */
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
}

void
draw_mod(float x, float y, float z, float width, float height, float depth)
{
	glPushMatrix();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(1.0f, 0.0f, 0.0f);

	/* Bottom */
	glVertex3f(x, y + height, z);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
	/* Top */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y, z);

	/* Front */
	glVertex3f(x, y + height, z);
	glVertex3f(x, y, z);
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y + height, z);
	/* Back */
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x, y + height, z + depth);

	/* Left */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x, y + height, z);
	/* Right */
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();
}

void
make_cluster(void)
{
	int r, cb, cg, m, n;
	float x = 0.0f, y = 0.0f, z = 0.0f;

	rowspace = 10.0f;
	cabspace = 5.0f;
	cagespace = 1.0f;
	modspace = 1.0f;

	modwidth = 1.0f;
	modheight = 2.0f;
	moddepth = 2.0f;

	nodespace = 0.2f;
	nodewidth = modwidth - 2.0f * nodespace;
	nodeheight = modheight - 4.0f * nodespace;
	nodedepth = modheight - 4.0f * nodespace;

	cageheight = modheight * 2.0f;
	cabwidth = (modwidth + modspace) * NMODS;
	rowdepth = moddepth * 2.0f;

	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
	glBegin(GL_QUADS);
	for (r = 0; r < NROWS; r++, z += rowdepth + rowspace) {
		for (cb = 0; cb < NCABS; cb++, x += cabwidth + cabspace) {
			for (cg = 0; cg < NCAGES; cg++, y += cageheight + cagespace) {
				for (m = 0; m < NMODS; m++, x += modwidth + modspace) {
					for (n = 0; n < NNODES; n++) {
						draw_node(&nodes[r][cb][cg][m][n],
						    x + nodespace,
						    y + (2.0f * nodespace + nodeheight) *
						      (n % (NNODES/2)) + nodespace,
						    z + (2.0f * nodespace + nodedepth) *
						      (n / (NNODES/2)) +
						      (nodespace * (n % (NNODES/2))),
						    nodewidth, nodeheight, nodedepth);
					}
//					draw_mod(x, y, z, modwidth,
//					    modheight, moddepth);
				}
				x -= (modwidth + modspace) * NMODS;
			}
			y -= (cageheight + cagespace) * NCAGES;
		}
		x -= (cabwidth + cabspace) * NCABS;
	}
end:
	glEnd();
	glEndList();
}

int
main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	if (glutCreateWindow("XT3 Monitor") == GL_FALSE)
		errx(1, "CreateWindow");
printf("x: white, y: blue, z: yellow\n");
/*
	glClearColor(0.5, 0.5, 0.5, 0.5);
*/
	glShadeModel(GL_SMOOTH);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);

	parse_physmap();
	parse_jobmap();
	make_cluster();

//	angle = -39.0;

	/* glutExposeFunc(reshape); */
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutSpecialFunc(sp_key);
	glutDisplayFunc(draw);
	glutIdleFunc(draw);
	glutMouseFunc(mouse);
	glutMotionFunc(active_m);
	glutPassiveMotionFunc(passive_m);

	glutMainLoop();

	exit(0);
}
