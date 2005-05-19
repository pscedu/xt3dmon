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
#include <string.h>
#include <unistd.h>

#include "mon.h"
#include "slist.h"

#define SLEEP_INTV	5
#define TRANS_INC	0.10

#define FOVY		(45.0f)
#define ASPECT		(win_width / (double)win_height)

#define SCALE		(1.0f)

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

#define ROWWIDTH	(CABWIDTH * NCABS + CABSPACE * (NCABS - 1))

#define XCENTER		(ROWWIDTH / 2)
#define YCENTER		((CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1)) / 2)
#define ZCENTER		((ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1)) / 2)

#define STARTX		(-150.0f)
#define STARTY		(30.0f)
#define STARTZ		(150.0f)

#define STARTLX		(1.0f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(0.0f)

#define WFRAMEWIDTH	(0.001f)

#define TWEEN_THRES	(0.01)
#define TWEEN_AMT	(.05)

struct lineseg {
	float		sx, sy, sz;
	float		ex, ey, ez;
	SLIST_ENTRY(lineseg) ln_next;
};

void		 make_cluster(void);
void		 load_textures(void);
void 		 del_textures(void);
void		 LoadTexture(void*, GLint, int);
void 		*LoadPNG(char*);

struct job	**jobs;
size_t		 njobs;
struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node	*invmap[NLOGIDS][NROWS * NCABS * NCAGES * NMODS * NNODES];
int		 op_tex = 0;
int		 op_blend = 0;
int		 op_wire = 1;
float		 op_alpha_job = 1.0f;
float		 op_alpha_oth = 1.0f;
GLint		 op_fmt = GL_RGBA;
float		 op_tween = TWEEN_AMT;
int		 op_lines = 1;
int		 op_env = 1;
int		 op_fps = 0;
int		 fps_active = 0;
int 		 op_ninfo = 1;
int		 win_width = 800;
int		 win_height = 600;

GLfloat 	 angle = 0.1f;
float		 x = STARTX, tx = STARTX, lx = STARTLX, tlx = STARTLX;
float		 y = STARTY, ty = STARTY, ly = STARTLY, tly = STARTLY;
float		 z = STARTZ, tz = STARTZ, lz = STARTLZ, tlz = STARTLZ;
int		 spkey, xpos, ypos;
GLint		 cluster_dl;
struct timeval	 lastsync;
long		 fps;
SLIST_HEAD(, lineseg) seglh;

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

	win_width = w;
	win_height = h;
	glViewport(0, 0, w, h);
	gluPerspective(FOVY, ASPECT, 1, 1000);
	glMatrixMode(GL_MODELVIEW);

	adjcam();
}

void
key(unsigned char key, int u, int v)
{
	switch (key) {
	case 'b':
		op_blend = !op_blend;
		break;
	case 'e':
		if (op_tween)
			op_tween = 0;
		else {
			op_tween = TWEEN_AMT;
			tx = x;  tlx = lx;
			ty = y;  tly = ly;
			tz = z;  tlz = lz;
		}
		break;
	case 'f':
		op_fps = !op_fps;
		fps_active = 1;
		break;
	case 'g':
		op_env = !op_env;
		break;
	case 'l': {
		struct lineseg *ln, *pln;

		op_lines = !op_lines;
		for (ln = SLIST_FIRST(&seglh); ln != NULL; ln = pln) {
			pln = SLIST_NEXT(ln, ln_next);
			free(ln);
		}
		SLIST_INIT(&seglh);
		break;
	    }
	case 'q':
		exit(0);
		/* NOTREACHED */
	case 't':
		op_tex = !op_tex;
		break;
	case 'T':
		op_fmt = ((op_fmt == GL_RGBA) ? GL_INTENSITY : GL_RGBA);
		del_textures();
		load_textures();
		break;
	case 'n':
		op_ninfo = !op_ninfo;
		break;
	case 'w':
		op_wire = !op_wire;
		break;
	case '+':
		op_alpha_job += ((op_alpha_job+TRANS_INC > 1.0) ? 0.0 : TRANS_INC);
		break;
	case '_':
		op_alpha_job -= ((op_alpha_job+TRANS_INC < 0.0) ? 0.0 : TRANS_INC);
		break;
	case '=':
		op_alpha_oth += ((op_alpha_oth+TRANS_INC > 1.0) ? 0.0 : TRANS_INC);
		break;
	case '-':
		op_alpha_oth -= ((op_alpha_oth+TRANS_INC < 0.0) ? 0.0 : TRANS_INC);
		break;
	}

	/* resync */
	glDeleteLists(cluster_dl, 1);
	make_cluster();
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
	float sx, sy, sz;
	float r = sqrt((x - XCENTER) * (x - XCENTER) +
	    (z - ZCENTER) * (z - ZCENTER));
	float adj = r / (ROWWIDTH / 2.0f);
	adj = pow(2, adj);
	if (adj > 50.0f)
		adj = 50.0f;

	sx = sy = sz = 0.0f; /* gcc */
	if (op_tween) {
		sx = x;  x = tx;
		sy = y;  y = ty;
		sz = z;  z = tz;
	}

	switch (key) {
	case GLUT_KEY_LEFT:
		x += lz * 0.3f * SCALE * adj;
		z -= lx * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_RIGHT:
		x -= lz * 0.3f * SCALE * adj;
		z += lx * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_UP:			/* Forward */
		x += lx * 0.3f * SCALE * adj;
		y += ly * 0.3f * SCALE * adj;
		z += lz * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_DOWN:			/* Backward */
		x -= lx * 0.3f * SCALE * adj;
		y -= ly * 0.3f * SCALE * adj;
		z -= lz * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_PAGE_UP:
		x += lx * ly * 0.3f * SCALE * adj;
		y += 0.3f * SCALE;
		z += lz * ly * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_PAGE_DOWN:
		x -= lx * ly * 0.3f * SCALE * adj;
		y -= 0.3f * SCALE;
		z -= lz * ly * 0.3f * SCALE * adj;
		break;
	default:
		return;
	}

	if (op_tween) {
		tx = x;  x = sx;
		ty = y;  y = sy;
		tz = z;  z = sz;
	} else
		adjcam();
}

/*
 * Node selection
 *
 * This assumes that the camera never "tilts" (rotates).
 */
void
sel_node(int u, int v)
{
	struct lineseg *ln;

	float r = sqrt((x - XCENTER) * (x - XCENTER) +
	    (z - ZCENTER) * (z - ZCENTER));
	float dist = r + ROWWIDTH;

	float adju = FOVY * ASPECT * 2.0f * PI / 360.0f * (u - win_width / 2.0f) / win_width;
	float angleu = acosf(lx);
	if (lz < 0)
		angleu = 2.0f * PI - angleu;
	float dx = cos(angleu + adju);
	float dz = sin(angleu + adju);

	float adjv = FOVY * 2.0f * PI / 360.0f * (win_height / 2.0f - v) / win_height;
	float anglev = asinf(ly);
//	if (ly < 0)
//		anglev = 2.0f * PI - anglev;
	float dy = sin(anglev + adjv);

	dx = x + dist * dx;
	dy = y + dist * dy;
	dz = z + dist * dz;

//printf("ly: %.2f, anglev: %.2f, adjv: %.2f\n", ly, anglev, adjv);

#if 0
printf("adju: [%.2f,%.2f] adjv: [%.2f,%.2f] r: %.2f src(%.2f,%.2f,%.2f) -> dst(%.2f,%.2f,%.2f)\n",
    adju, FOVY * ASPECT * 2.0f * PI / 360.0f,
    adjv, FOVY * 2.0f * PI / 360.0f,
    r, x, y, z, dx, dy, dz
    );
#endif

	if ((ln = malloc(sizeof(*ln))) == NULL)
		err(1, NULL);
	ln->sx = x;
	ln->sy = y;
	ln->sz = z;

	ln->ex = dx;
	ln->ey = dy;
	ln->ez = dz;
	SLIST_INSERT_HEAD(&seglh, ln, ln_next);
}

void
mouse(int button, int state, int u, int v)
{
	spkey = glutGetModifiers();
	xpos = u;
	ypos = v;
	if (!spkey)
		sel_node(u, v);
}

void
active_m(int u, int v)
{
	int du = u - xpos, dv = v - ypos;
	float t, r;
	float sx, sy, sz, slx, sly, slz;

	xpos = u;
	ypos = v;

	sx = sy = sz = 0.0f; /* gcc */
	slx = sly = slz = 0.0f; /* gcc */
	if (op_tween) {
		sx = x;  x = tx;
		sy = y;  y = ty;
		sz = z;  z = tz;

		slx = lx;  lx = tlx;
		sly = ly;  ly = tly;
		slz = lz;  lz = tlz;
	}

	if (du != 0 && spkey & GLUT_ACTIVE_CTRL) {
		r = sqrt((x - XCENTER) * (x - XCENTER) +
		    (z - ZCENTER) * (z - ZCENTER));
		t = acosf((x - XCENTER) / r);
		if (z < ZCENTER)
			t = 2.0f * PI - t;
		t += .01f * (float)du;
		if (t < 0)
			t += PI * 2.0f;

		x = r * cos(t) + XCENTER;
		z = r * sin(t) + ZCENTER;
		lx = (XCENTER - x) / r;
		lz = (ZCENTER - z) / r;
	}
	if (dv != 0 && spkey & GLUT_ACTIVE_SHIFT) {
		float adj = (dv < 0) ? 0.005f : -0.005f;
		if (fabs(angle + adj) < PI / 2.0f) {
			angle += adj;
			ly = sin(angle);
		}
	}
	float mag = sqrt(lx*lx + ly*ly + lz*lz);
	lx /= mag;
	ly /= mag;
	lz /= mag;
	mag = sqrt(lx*lx + ly*ly + lz*lz);

	if (op_tween) {
		tx = x;  x = sx;
		ty = y;  y = sy;
		tz = z;  z = sz;

		tlx = lx;  lx = slx;
		tly = ly;  ly = sly;
		tlz = lz;  lz = slz;
	} else
		adjcam();
}

void
passive_m(int u, int v)
{
	xpos = u;
	ypos = v;
}

#define FPS_STRING 10
#define TEXT_HEIGHT 25
#define X_SPEED 2

void
draw_fps(int on)
{
	int i;
	char frate[FPS_STRING];
	double x, y, w, h;
	double cx, cy;
	int vp[4];
	static double sx = 0;
	static double sy = 0;

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

	/* Create String */
	memset(frate, '\0', FPS_STRING);
	snprintf(frate, sizeof(frate), "FPS: %ld", fps);

	/* Coordinates */
	w = sizeof(frate)*8;
	h = TEXT_HEIGHT;
	x = vp[2] - w;
	y = vp[3];

	/*
	** Adjust current pos, on = 1, move onto screen
	** off = 0, move off screen */
	if(sx == 0)
		sx = vp[2];
	else if(sx > x - 1 && on)
		sx -= X_SPEED;
	else if(sx < vp[2]+1 && !on)
		sx += X_SPEED;
	else
		fps_active = 0;

	/* Draw the frame rate */
	glColor4f(1.0,1.0,1.0,1.0);

	/* Account for text height, and fps digit len */
	cx = (fps > 999 ? 0 : 8);
	cy = 8;

	glRasterPos2d(sx+cx,y-TEXT_HEIGHT+cy);


	for(i = 0; i < sizeof(frate); i++) 
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, frate[i]);

	/* Draw polygon/wireframe around fps */
	for(i = 0; i < 2; i++){
	
		if(i == 0){
			glBegin(GL_POLYGON);
			glColor4f(0.20, 0.40, 0.5, 1.0);
		}else{
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

#define MAX_INFO 50
#define INFO_ITEMS 4

int maxlen(char *str[MAX_INFO], int size)
{
	int len;
	int t, i;

	for(i = 0, t = 0; i < size; i++)
	{
		len = (t > strlen(str[i]) ? t: strlen(str[i]));
		t = len;
	}
	
	return len;
}

void
draw_node_info(void)
{
	int i, j;
	int vp[4];
	double x, y, w, h;
	double cy;
	char str[INFO_ITEMS][MAX_INFO] = {
	"Owner: %s", "Job Name: %s", "Duration: %d", "Number CPU's: %d"};
	static int len = 0;

	/* TODO: snprintf any data into our above strings */

	/* Calculate the max string length */
	if(len == 0)
		for(i = 0, j = 0; i < INFO_ITEMS; i++) {
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

	/* Take into account draw_fps (1 Row) */
	w = len*8;
	h = INFO_ITEMS*TEXT_HEIGHT;
	x = vp[2] - w;
	y = vp[3] - 1.25*TEXT_HEIGHT;

	/* Factor to center text on y axis */
	cy = 8;

	/* Draw Node Info */
	glColor4f(1.0,1.0,1.0,1.0);
	for(i = 0; i < INFO_ITEMS; i++) {
	

		/* Set the position of the text (account for fps!) */
		glRasterPos2d(x, (y-(i+1)*TEXT_HEIGHT)+cy);

		for(j = 0; j < sizeof(str[i]); j++) 
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, str[i][j]);
	}

	/* Draw a Polygon around node legend */
	for(i = 0; i < 2; i++){
	
		if(i == 0){
			glBegin(GL_POLYGON);
			glColor4f(0.20, 0.40, 0.5, 1.0);
		}else{
			glLineWidth(2.0);
			glBegin(GL_LINE_LOOP);
			glColor4f(0.40, 0.80, 1.0, 1.0);
		}

		/* Draw the Polygon around the framerate */
		glVertex2d(x, y);
		glVertex2d(x+w, y);
		glVertex2d(x+w, y-h);
		glVertex2d(x, y-h);

		glEnd();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

void
draw(void)
{
	struct lineseg *ln;

	if (op_tween && (tx - x || ty - y || tz - z ||
	    tlx - lx || tly - ly || tlz - lz)) {
		x += (tx - x) * op_tween;
		y += (ty - y) * op_tween;
		z += (tz - z) * op_tween;
		if (fabs(tx - x) < TWEEN_THRES)
			x = tx;
		if (fabs(ty - y) < TWEEN_THRES)
			y = ty;
		if (fabs(tz - z) < TWEEN_THRES)
			z = tz;

		lx += (tlx - lx) * op_tween;
		ly += (tly - ly) * op_tween;
		lz += (tlz - lz) * op_tween;
		if (fabs(tlx - lx) < TWEEN_THRES)
			lx = tlx;
		if (fabs(tly - ly) < TWEEN_THRES)
			ly = tly;
		if (fabs(tlz - lz) < TWEEN_THRES)
			lz = tlz;
		adjcam();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (op_env) {
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
	}

	SLIST_FOREACH(&seglh, ln, ln_next) {
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_LINES);
		glLineWidth(2.0f);
		glVertex3f(ln->sx, ln->sy, ln->sz);
		glVertex3f(ln->ex, ln->ey, ln->ez);
		glEnd();
	}

	if(op_ninfo)
		draw_node_info();
	if(op_fps || fps_active)
		draw_fps((op_fps));

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
		if (tv.tv_sec - lastsync.tv_sec) {
			fps = tcnt / (tv.tv_sec - lastsync.tv_sec);
  //			printf("fps: %ld\n", fps);
		}
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
	x -= WFRAMEWIDTH;
	y -= WFRAMEWIDTH;
	z -= WFRAMEWIDTH;
	width += 2.0f * WFRAMEWIDTH;
	height += 2.0f * WFRAMEWIDTH;
	depth += 2.0f * WFRAMEWIDTH;

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
		LoadTexture(data, op_fmt, i + 1);
		states[i].st_texid = i + 1;
	}
}

void
del_textures(void)
{
	int i;

	/* Delete textures from vid memory */
	for (i = 0; i < NST; i++)
		glDeleteTextures(1, &states[i].st_texid);
}

int
main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(win_width, win_height);
	if (glutCreateWindow("XT3 Monitor") == GL_FALSE)
		errx(1, "CreateWindow");

	glShadeModel(GL_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);

	SLIST_INIT(&seglh);

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
