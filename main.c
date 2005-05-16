/* $Id$ */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>

#if defined(__APPLE_CC__)
#include<OpenGL/gl.h>
#include<GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/freeglut.h>
#endif

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mon.h"

#define WIN_WIDTH	800
#define WIN_HEIGHT	600

#define SLEEP_INTV	5

#define SCALE (1.0f)

#define ROWSPACE	((10.0f)*SCALE)
#define CABSPACE	((5.0f)*SCALE)
#define CAGESPACE	((1.0f)*SCALE)
#define MODSPACE	((1.0f)*SCALE)

#define MODWIDTH	((1.0f)*SCALE)
#define MODHEIGHT	((2.0f)*SCALE)
#define MODDEPTH	((2.0f)*SCALE)

#define NODESPACE	((0.2f)*SCALE)
#define NODEWIDTH	(MODWIDTH - 2.0f * NODESPACE)
#define NODEHEIGHT	(MODHEIGHT - 4.0f * NODESPACE)
#define NODEDEPTH	(MODHEIGHT - 4.0f * NODESPACE)

#define CAGEHEIGHT	(MODHEIGHT * 2.0f)
#define CABWIDTH	((MODWIDTH + MODSPACE) * NMODS)
#define ROWDEPTH	(MODDEPTH * 2.0f)

#define XORIGIN		((CABWIDTH * NCABS + CABSPACE * (NCABS - 1)) / 2)
#define YORIGIN		((CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1)) / 2)
#define ZORIGIN		((ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1)) / 2)

#define FRAMEWIDTH	(0.001f)

void		 make_cluster(void);

struct job	**jobs;
size_t		 njobs;
struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node	*invmap[NLOGIDS][NROWS * NCABS * NCAGES * NMODS * NNODES];
int		 op_tex = 1;
int		 op_blend = 0;
int		 op_wire = 1;
float		 op_alpha_job = 1.0f;
float		 op_alpha_oth = 1.0f;

GLfloat 	 angle = 0.1f;
float		 x = -15.0f, y = 9.0f, z = 15.0f;
float		 lx = 0.9f, ly = 0.0f, lz = -0.3f;
int		 spkey, xpos, ypos;
GLint		 cluster_dl;
struct timeval	 lastsync;

struct state states[] = {
	{ "Free",		1.0, 1.0, 1.0, 1 },
	{ "Disabled (PBS)",	1.0, 0.0, 0.0, 1 },
	{ "Disabled (HW)",	0.66, 0.66, 0.66, 1 },
	{ NULL,			0.0, 0.0, 0.0, 1 },
	{ "I/O",		1.0, 1.0, 0.0, 1 },
	{ "Unaccounted",	0.0, 0.0, 1.0, 1 },
	{ "Bad",		1.0, 0.75, 0.75, 1 },
	{ "Checking",		0.0, 1.0, 0.0, 1 }
};

__inline void
adjcam(void)
{
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
 *    z
 *   /  Up  /|
 *  / Down |/
 * +------------------- x
 * |  Left <-  -> Right
 * |
 * |   PageUp /\
 * | PageDown \/
 * y
 */
void
sp_key(int key, int u, int v)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		x += lz * 0.3f * SCALE;
		z -= lx * 0.3f * SCALE;
		break;
	case GLUT_KEY_RIGHT:
		x -= lz * 0.3f * SCALE;
		z += lx * 0.3f * SCALE;
		break;
	case GLUT_KEY_UP:
		x += lx * 0.3f * SCALE;
		z += lz * 0.3f * SCALE;
		break;
	case GLUT_KEY_DOWN:
		x -= lx * 0.3f * SCALE;
		z -= lz * 0.3f * SCALE;
		break;
	case GLUT_KEY_PAGE_UP:
		y += 0.3f * SCALE;
		break;
	case GLUT_KEY_PAGE_DOWN:
		y -= 0.3f * SCALE;
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

	if (du != 0 && spkey & GLUT_ACTIVE_CTRL) {
		r = sqrt((x - XORIGIN) * (x - XORIGIN) +
		    (z - ZORIGIN) * (z - ZORIGIN));
		t = acosf((x - XORIGIN) / r);
		if (z < ZORIGIN)
			t = 2.0f * PI - t;
		t += .01 * (float)du;
		if (t < 0)
			t += PI * 2.0f;

		x = r * cos(t) + XORIGIN;
		z = r * sin(t) + ZORIGIN;
		lx = (XORIGIN - x) / r;
		lz = (ZORIGIN - z) / r;
	}
	if (dv != 0 && spkey & GLUT_ACTIVE_SHIFT) {
		angle += (dv < 0) ? 0.005f : -0.005f;
		ly = sin(angle);
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	glCallList(cluster_dl);
	glutSwapBuffers();
}

#define MAXCNT 100

void
idle(void)
{
	static int cnt = 0;
	static int tcnt = 0;

	tcnt++;
	if (cnt++ >= MAXCNT) {
		struct timeval tv;

		if (gettimeofday(&tv, NULL) == -1)
			err(1, "gettimeofday");
if (tv.tv_sec - lastsync.tv_sec)
  printf("fps: %ld\n", tcnt / (tv.tv_sec - lastsync.tv_sec));
		if (lastsync.tv_sec + SLEEP_INTV < tv.tv_sec) {
			tcnt = 0;
			lastsync.tv_sec = tv.tv_sec;
			glDeleteLists(cluster_dl, 1);
			parse_jobmap();
			make_cluster();
		}
		cnt = 0;
	}
	draw();
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
draw_filled_node(struct node *n, float x, float y, float z, float width,
    float height, float depth)
{
	float r, g, b, a;

	if (n->n_state == ST_USED) {
		r = n->n_job->j_r;
		g = n->n_job->j_g;
		b = n->n_job->j_b;
		a = op_alpha_job;
	} else {
		r = states[n->n_state].st_r;
		g = states[n->n_state].st_g;
		b = states[n->n_state].st_b;
		a = op_alpha_oth;
	}

	if (op_blend) {
		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
	} else
		a = 1.0;

	glColor4f(r, g, b, a);
	glBegin(GL_POLYGON);
	/* Bottom */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+depth);		/*  1 */
	glVertex3f(x+width, y, z+depth);	/*  2 */
	glVertex3f(x+width, y, z);		/*  3 */
	glVertex3f(x, y, z);			/*  4 */
	/* Back */
	glVertex3f(x, y+height, z);		/*  5 */
	glVertex3f(x, y+height, z+depth);	/*  6 */
	glVertex3f(x, y, z+depth);		/*  7 */
	/* Right */
	glVertex3f(x+width, y, z+depth);	/*  8 */
	glVertex3f(x+width, y+height, z+depth);	/*  9 */
	glVertex3f(x, y+height, z+depth);	/* 10 */

	glVertex3f(x, y+height, z);		/* 11 */

	/* Left */
	glVertex3f(x+width, y+height, z);	/* 12 */
	glVertex3f(x+width, y, z);		/* 13 */
	glVertex3f(x+width, y+height, z);	/* 14 */
	/* Front */
	glVertex3f(x+width, y+height, z+depth);	/* 15 */
	glEnd();

	if (op_blend)
		glDisable(GL_BLEND);
}

__inline void
draw_wireframe_node(struct node *n, float x, float y, float z, float width,
    float height, float depth)
{
	/* Wireframe outline */
	x -= FRAMEWIDTH;
	y -= FRAMEWIDTH;
	z -= FRAMEWIDTH;
	width += 2.0f * FRAMEWIDTH;
	height += 2.0f * FRAMEWIDTH;
	depth += 2.0f * FRAMEWIDTH;

	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	/* Bottom */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+depth);		/*  1 */
	glVertex3f(x+width, y, z+depth);	/*  2 */
	glVertex3f(x+width, y, z);		/*  3 */
	glVertex3f(x, y, z);			/*  4 */
	/* Back */
	glVertex3f(x, y+height, z);		/*  5 */
	glVertex3f(x, y+height, z+depth);	/*  6 */
	glVertex3f(x, y, z+depth);		/*  7 */
	/* Right */
	glVertex3f(x+width, y, z+depth);	/*  8 */
	glVertex3f(x+width, y+height, z+depth);	/*  9 */
	glVertex3f(x, y+height, z+depth);	/* 10 */

	glVertex3f(x, y+height, z);		/* 11 */

	/* Left */
	glVertex3f(x+width, y+height, z);	/* 12 */
	glVertex3f(x+width, y, z);		/* 13 */
	glVertex3f(x+width, y+height, z);	/* 14 */
	/* Front */
	glVertex3f(x+width, y+height, z+depth);	/* 15 */
	glEnd();
}


__inline void
draw_textured_node(struct node *n, float x, float y, float z, float width,
    float height, float depth)
{
//	float ux, uy, uz;
	float uw, uh, ud;
	float color[4];
	GLenum param;

	/* Convert to texture Units */
#if 0
	/* Too Big */
	ud = depth / TEX_SIZE;
	uh = height / TEX_SIZE;
	uw = width / TEX_SIZE;

	/* Too Small*/
	ud = depth;
	uh = height;
	uw = width;
#endif

	uw = 1.0;
	ud = 2.0;
	uh = 2.0;

//	ux = 0.0;
//	uy = 0.0;
//	uz = 0.0;

	glEnable(GL_TEXTURE_2D);

	/* DEBUG */
	if (op_blend){
		glEnable(GL_BLEND);

		/* 1 */
		//glBlendFunc(GL_SRC_COLOR, GL_CONSTANT_ALPHA);

		/* 2 Works with: GL_INTENSITY */
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);

		/* 3 Transparent, but alpha value doesn't effect */
		//glBlendFunc(GL_SRC_COLOR, GL_DST_ALPHA);

		/* 4 Works with: GL_INTENSITY */
		//glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

		param = GL_BLEND;
	} else
		param = GL_REPLACE;

	glBindTexture(GL_TEXTURE_2D, states[n->n_state].st_texid);

	if (n->n_state == ST_USED) {
		color[0] = n->n_job->j_r;
		color[1] = n->n_job->j_g;
		color[2] = n->n_job->j_b;
		color[3] = op_alpha_job;
	} else {
		/* Default color, with alpha */
		color[0] = 0.90;
		color[1] = 0.80;
		color[2] = 0.50;
		color[3] = op_alpha_oth;
	}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, param);

	if (op_blend)
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);


	/* Back  */
	glBegin(GL_POLYGON);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+height, z);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+width, y+height, z);
	glTexCoord2f(uh, uw);
	glVertex3f(x+width, y, z);
	glTexCoord2f(uh, 0.0);
	glEnd();


	/* Front */
	glBegin(GL_POLYGON);

#if 0
	Same as Below, execpt using 3f
	glVertex3f(x, y, z+depth);
	glTexCoord3f(0.0, 0.0, 0.0);
	glVertex3f(x, y+height, z+depth);
	glTexCoord3f(0.0, uw, 0.0);
	glVertex3f(x+width, y+height, z+depth);
	glTexCoord3f(uh, uw, 0.0);
	glVertex3f(x+width, y, z+depth);
	glTexCoord3f(uh, 0.0, 0.0);
#endif

	glVertex3f(x, y, z+depth);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+height, z+depth);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+width, y+height, z+depth);
	glTexCoord2f(uh, uw);
	glVertex3f(x+width, y, z+depth);
	glTexCoord2f(uh, 0.0);
	glEnd();

	/* Right */
	glBegin(GL_POLYGON);
	glVertex3f(x+width, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x+width, y, z+depth);
	glTexCoord2f(0.0, ud);
	glVertex3f(x+width, y+height, z+depth);
	glTexCoord2f(uh, ud);
	glVertex3f(x+width, y+height, z);
	glTexCoord2f(uh, 0.0);
	glEnd();

	/* Left */
	glBegin(GL_POLYGON);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+depth);
	glTexCoord2f(0.0, ud);
	glVertex3f(x, y+height, z+depth);
	glTexCoord2f(uh, ud);
	glVertex3f(x, y+height, z);
	glTexCoord2f(uh, 0.0);
	glEnd();

	/* Top */
	glBegin(GL_POLYGON);
	glVertex3f(x, y+height, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+height, z+depth);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+width, y+height, z+depth);
	glTexCoord2f(ud, uw);
	glVertex3f(x+width, y+height, z);
	glTexCoord2f(ud, 0.0);
	glEnd();

	/* Bottom */
	glBegin(GL_POLYGON);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+depth);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+width, y, z+depth);
	glTexCoord2f(ud, uw);
	glVertex3f(x+width, y, z);
	glTexCoord2f(ud, 0.0);
	glEnd();

	/* DEBUG */
	if (op_blend)
		glDisable(GL_BLEND);

	glDisable(GL_TEXTURE_2D);
}

__inline void
draw_node(struct node *n, float x, float y, float z, float width,
    float height, float depth)
{
	if (op_tex)
		draw_textured_node(n, x, y, z, width, height, depth);
	else
		draw_filled_node(n, x, y, z, width, height, depth);

	if (op_wire)
		draw_wireframe_node(n, x, y, z, width, height, depth);
}

void
make_cluster(void)
{
	int r, cb, cg, m, n;
	float x = 0.0f, y = 0.0f, z = 0.0f;

	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
	for (r = 0; r < NROWS; r++, z += ROWDEPTH + ROWSPACE) {
		for (cb = 0; cb < NCABS; cb++, x += CABWIDTH + CABSPACE) {
			for (cg = 0; cg < NCAGES; cg++, y += CAGEHEIGHT + CAGESPACE) {
				for (m = 0; m < NMODS; m++, x += MODWIDTH + MODSPACE) {
					for (n = 0; n < NNODES; n++) {
						draw_node(&nodes[r][cb][cg][m][n],
						    x + NODESPACE,
						    y + (2.0f * NODESPACE + NODEHEIGHT) *
						      (n % (NNODES/2)) + NODESPACE,
						    z + (2.0f * NODESPACE + NODEDEPTH) *
						      (n / (NNODES/2)) +
						      (NODESPACE * (n % (NNODES/2))),
						    NODEWIDTH, NODEHEIGHT, NODEDEPTH);
					}
				}
				x -= (MODWIDTH + MODSPACE) * NMODS;
			}
			y -= (CAGEHEIGHT + CAGESPACE) * NCAGES;
		}
		x -= (CABWIDTH + CABSPACE) * NCABS;
	}
end:
	glEndList();
}

void
load_textures(void)
{
	char path[NAME_MAX];
	void *data;
	int i;

	/* Read in texture IDs */
	for (i = 0; i < NST; i++) {
		snprintf(path, sizeof(path), _PATH_TEX, i);
		data = LoadPNG(path);
		LoadTexture(data, i + 1);
		states[i].st_texid = i + 1;
	}
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

	glShadeModel(GL_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);

	load_textures();
	parse_physmap();
	parse_jobmap();
	make_cluster();

	/* glutExposeFunc(reshape); */
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutSpecialFunc(sp_key);
	glutDisplayFunc(draw);
	glutIdleFunc(idle);
	glutMouseFunc(mouse);
	glutMotionFunc(active_m);
	glutPassiveMotionFunc(passive_m);
	glutMainLoop();
	exit(0);
}
