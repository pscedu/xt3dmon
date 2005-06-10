/* $Id$ */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cdefs.h"
#include "mon.h"
#include "buf.h"

#define SLEEP_INTV	5

#define FOVY		(45.0f)
#define ASPECT		(win_width / (double)win_height)

#define STARTX		(-30.0f)
#define STARTY		(10.0f)
#define STARTZ		(25.0f)

#define STARTLX		(0.99f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NLOGIDS][NROWS * NCABS * NCAGES * NMODS * NNODES];
int			 win_width = 800;
int			 win_height = 600;
int			 active_flyby = 0;
int			 build_flyby = 0;

float			 tx = STARTX, tlx = STARTLX, ox = STARTX, olx = STARTLX;
float			 ty = STARTY, tly = STARTLY, oy = STARTY, oly = STARTLY;
float			 tz = STARTZ, tlz = STARTLZ, oz = STARTZ, olz = STARTLZ;
int			 spkey, lastu, lastv;
GLint			 cluster_dl;
struct timeval		 lastsync;
long			 fps;

struct state st = {
	STARTX, STARTY, STARTZ,				/* (x,y,z) */
	STARTLX, STARTLY, STARTLZ,			/* (lx,ly,lz) */
	OP_WIRES | OP_TWEEN | OP_GROUND | OP_DISPLAY,	/* options */
	1.0f,						/* job alpha */
	1.0f,						/* other alpha */
	GL_RGBA,					/* alpha blend format */
	SM_JOBS,					/* which data to show */
	VM_PHYSICAL,					/* viewing mode */
	NULL,						/* selected node */
	0,						/* panels (unused) */
	0,						/* tween mode (unused) */
	0,						/* nframes (unused) */
	0						/* rebuild flags (unused) */
};

struct job_state jstates[] = {
	{ "Free",		{ 1.00f, 1.00f, 1.00f, 1.00f, 0 } },	/* White */
	{ "Disabled (PBS)",	{ 1.00f, 0.00f, 0.00f, 1.00f, 0 } },	/* Red */
	{ "Disabled (HW)",	{ 0.66f, 0.66f, 0.66f, 1.00f, 0 } },	/* Gray */
	{ NULL,			{ 0.00f, 0.00f, 0.00f, 1.00f, 0 } },	/* (dynamic) */
	{ "I/O",		{ 1.00f, 1.00f, 0.00f, 1.00f, 0 } },	/* Yellow */
	{ "Unaccounted",	{ 0.00f, 0.00f, 1.00f, 1.00f, 0 } },	/* Blue */
	{ "Bad",		{ 1.00f, 0.75f, 0.75f, 1.00f, 0 } },	/* Pink */
	{ "Checking",		{ 0.00f, 1.00f, 0.00f, 1.00f, 0 } },	/* Green */
	{ "Info",		{ 0.20f, 0.40f, 0.60f, 1.00f, 0 } }	/* Dark blue */
};

#if 0
void
calc_flyby(void)
{
	static const struct state *curst = NULL;
	static struct state lastst;
	float frac;

	if (curst == NULL) {
printf("start\n");
		curst = flybypath;
		lastst = st;
		lastst.st_nframes = 1;
	} else if (++lastst.st_nframes > curst->st_nframes) {
printf("next state\n");
		if ((++curst)->st_nframes == 0) {
printf("done\n");
			active_flyby = 0;
			curst = NULL;
			return;
		}
		lastst.st_nframes = 1;
	}
	if (lastst.st_nframes == 1) {
		st.st_opts = curst->st_opts;
		st.st_panels = curst->st_panels;
		st.st_alpha_fmt = curst->st_alpha_fmt;
		st.st_alpha_job = curst->st_alpha_job;
		st.st_alpha_oth = curst->st_alpha_oth;
		if (st.st_opts & OP_TWEEN) {
			tx = curst->st_x;
			ty = curst->st_y;
			tz = curst->st_z;
			tlx = curst->st_lx;
			tly = curst->st_ly;
			tlz = curst->st_lz;
			/* tween_amt = 1 / (float)curst->st_nframes; */
		}
	}
	if (st.st_opts & OP_TWEEN)
		return;
if(0)
printf("(%.3f,%.3f,%.3f)->(%.3f,%.3f,%.3f)->(%.3f,%.3f,%.3f) ",
  lastst.st_x, lastst.st_y, lastst.st_z,
  st.st_x, st.st_y, st.st_z,
  curst->st_x, curst->st_y, curst->st_z);

//	frac = (lastst.st_nframes / (float)curst->st_nframes);
	frac = curst->st_nframes;
//printf("xadj: %.2f\n", (curst->st_x - lastst.st_x) / frac);

if (0)
printf("(%.3f,%.3f,%.3f):(%.3f,%.3f,%.3f) ",
  st.st_x, st.st_y, st.st_z,
  st.st_lx, st.st_ly, st.st_lz);

//printf("."); fflush(stdout);
	st.st_x += (curst->st_x - lastst.st_x) / frac;
	st.st_y += (curst->st_y - lastst.st_y) / frac;
	st.st_z += (curst->st_z - lastst.st_z) / frac;
	st.st_lx += (curst->st_lx - lastst.st_lx) / frac;
	st.st_ly += (curst->st_ly - lastst.st_ly) / frac;
	st.st_lz += (curst->st_lz - lastst.st_lz) / frac;
if (0)
printf("(%.3f,%.3f,%.3f):(%.3f,%.3f,%.3f)\n",
  st.st_x, st.st_y, st.st_z,
  st.st_lx, st.st_ly, st.st_lz);

if(0)
printf("+(%.3f,%.3f,%.3f) l(%.3f,%.3f,%.3f)\n",
       (curst->st_x - lastst.st_x) / frac,
       (curst->st_y - lastst.st_y) / frac,
       (curst->st_z - lastst.st_z) / frac,
        (curst->st_lx - lastst.st_lx) / frac,
        (curst->st_ly - lastst.st_ly) / frac,
        (curst->st_lz - lastst.st_lz) / frac);

	adjcam();
}
#endif

void
adjcam(void)
{
	glLoadIdentity();
	gluLookAt(st.st_x, st.st_y, st.st_z,
	    st.st_x + st.st_lx,
	    st.st_y + st.st_ly,
	    st.st_z + st.st_lz,
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

void restore_state(int flyby)
{
	/* Restore Tweening state */
	if (!(st.st_opts & OP_TWEEN)) {
		ox = tx = st.st_x;  olx = tlx = st.st_lx;
		oy = ty = st.st_y;  oly = tly = st.st_ly;
		oz = tz = st.st_z;  olz = tlz = st.st_lz;
	}

	/* 
	** Below: States that require make_cluster
	** ---------------------------------------
	*/
	
	/* Restore Selected Node */
#if 0
	if (selnode != NULL) {
		selnode->n_state = selnode->n_savst;
		selnode = NULL;
	}
#endif
 	if (st.st_ro)
		rebuild(st.st_ro);

	if (flyby){
		st.st_ro = 0;
	}
}

void
select_node(struct node *n)
{
	if (st.st_selnode == n)
		return;
	if (st.st_selnode != NULL)
		st.st_selnode->n_state = st.st_selnode->n_savst;
	st.st_selnode = n;
	if (n->n_state != JST_INFO)
		n->n_savst = n->n_state;
	n->n_state = JST_INFO;
	rebuild(RO_COMPILE);
}

/*
 * y
 * |        (x+w,y+h,z)    (x+w,y+h,z+d)
 * |            -----------------
 * |           /               /
 * |          /               /
 * |         -----------------
 * |    (x,y+h,z)       (x,y+h,z+d)
 * |
 * |             +--------------+
 * |     /|      |              |     /|
 * |    / |      |              |    / |
 * |   /  |      |              |   /  |
 * |  /   |      |              |  /   |
 * | |    |  +--------------+   | |    |
 * | |    |  |      / \     |   | |    |
 * | |    |  |       |      |   | |    |
 * | |    |  |     lt_y     |   | |    |
 * | |    |  |       |      |   | |    |
 * | |    |  |       |      |---+ |    |
 * | |   /   |<--- lt_z --->|     |   /
 * | |  /    |       |      |     |  /
 * | | /     |       |      |     | /
 * | |/      |      \ /     |     |/
 * |         +--------------+
 * |      x
 * |     /  (x+w,y,z)       (x+w,y,z+d)
 * |    /       -----------------
 * |   /       /               /
 * |  /       /               /
 * | /       -----------------
 * |/    (x,y,z)         (x,y,z+d)
 * +------------------------------------- z
 */
__inline int
collide(struct node *n,
   float w, float h, float d,
   float sx, float sy, float sz,
   float ex, float ey, float ez)
{
	float x = n->n_pos.np_x;
	float y = n->n_pos.np_y;
	float z = n->n_pos.np_z;
	float lo_x, hi_x;
	float lo_y, hi_y, y_sxbox, y_exbox;
	float lo_z, hi_z, z_sxbox, z_exbox;

	if (ex < sx) {
		lo_x = ex;
		hi_x = sx;
	} else {
		lo_x = sx;
		hi_x = ex;
	}

	if (hi_x < x || lo_x > x+w)
		return (0);
	y_sxbox = sy + ((ey - sy) / (ex - sx)) * (x - sx);
	y_exbox = sy + ((ey - sy) / (ex - sx)) * (x+h - sx);

	if (y_exbox < y_sxbox) {
		lo_y = y_exbox;
		hi_y = y_sxbox;
	} else {
		lo_y = y_sxbox;
		hi_y = y_exbox;
	}

	if (hi_y < y || lo_y > y+h)
		return (0);
	z_sxbox = sz + ((ez - sz) / (ex - sx)) * (x - sx);
	z_exbox = sz + ((ez - sz) / (ex - sx)) * (x+w - sx);

	if (z_exbox < z_sxbox) {
		lo_z = z_exbox;
		hi_z = z_sxbox;
	} else {
		lo_z = z_sxbox;
		hi_z = z_exbox;
	}

	if (hi_z < z || lo_z > z+d)
		return (0);
	select_node(n);
	return (1);
}

/* Node detection */
void
detect_node(int u, int v)
{
	GLint vp[4];
	GLdouble mvm[16];
	GLdouble pvm[16];
	GLint x, y, z;
	GLdouble sx, sy, sz;
	GLdouble ex, ey, ez;

	int r, cb, cg, m, n;
	int pr, pcb, pcg, pm;
	int n0, n1, pn;

	/* Grab world info */
	glGetIntegerv(GL_VIEWPORT, vp);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
	glGetDoublev(GL_PROJECTION_MATRIX, pvm);

	/* Fix y coordinate */
	y = vp[3] - v - 1;
	x = u;

	/* Transform 2d to 3d coordinates according to z */
	z = 0.0;
	gluUnProject(x, y, z, mvm, pvm, vp, &sx, &sy, &sz);

	z = 1.0;
	gluUnProject(x, y, z, mvm, pvm, vp, &ex, &ey, &ez);

	/* Check for collision */
	for (r = 0; r < NROWS; r++) {
		for (cb = 0; cb < NCABS; cb++) {
			for (cg = 0; cg < NCAGES; cg++) {
				for (m = 0; m < NMODS; m++) {
					for (n = 0; n < NNODES; n++) {
						n0 = st.st_lz > 0.0f ? n : NNODES - n - 1;
						n1 = st.st_ly > 0.0f ? n : NNODES - n - 1;
						pn = 2 * (n0 / (NNODES / 2)) +
						    n1 % (NNODES / 2);

						pr  = st.st_lz > 0.0f ? r  : NROWS  - r  - 1;
						pcb = st.st_lx > 0.0f ? cb : NCABS  - cb - 1;
						pcg = st.st_ly > 0.0f ? cg : NCAGES - cg - 1;
						pm  = st.st_lx > 0.0f ? m  : NMODS  - m  - 1;

						if (collide(&nodes[pr][pcb][pcg][pm][pn],
						    NODEWIDTH, NODEHEIGHT, NODEDEPTH,
						   sx, sy, sz, ex, ey, ez))
							return;
					}
				}
			}
		}
	}
}

void
mouse(__unused int button, __unused int state, int u, int v)
{
	if (active_flyby)
		return;
printf("mouse()\n");
	spkey = glutGetModifiers();
	lastu = u;
	lastv = v;
}

void
active_m(int u, int v)
{
	int du = u - lastu, dv = v - lastv;
	float sx, sy, sz, slx, sly, slz;
	float adj, t, r, mag;

	if (active_flyby)
		return;

	if (abs(du) + abs(dv) <= 1)
		return;

printf("[%d,%d]\n", u, v);
	lastu = u;
	lastv = v;

	sx = sy = sz = 0.0f; /* gcc */
	slx = sly = slz = 0.0f; /* gcc */
	if (st.st_opts & OP_TWEEN) {
		ox = st.st_x;  olx = st.st_lx;
		oy = st.st_y;  oly = st.st_ly;
		oz = st.st_z;  olz = st.st_lz;

		sx = st.st_x;  st.st_x = tx;
		sy = st.st_y;  st.st_y = ty;
		sz = st.st_z;  st.st_z = tz;

		slx = st.st_lx;  st.st_lx = tlx;
		sly = st.st_ly;  st.st_ly = tly;
		slz = st.st_lz;  st.st_lz = tlz;
	}

	if (du != 0 && spkey & GLUT_ACTIVE_CTRL) {
		r = sqrt(SQUARE(st.st_x - XCENTER) +
		    SQUARE(st.st_z - ZCENTER));
		t = acosf((st.st_x - XCENTER) / r);
		if (st.st_z < ZCENTER)
			t = 2.0f * PI - t;
		t += .01f * (float)du;
		if (t < 0)
			t += PI * 2.0f;

		/*
		 * Maintain the magnitude of lx*lx + lz*lz.
		 */
		mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_lz));
		st.st_x = r * cos(t) + XCENTER;
		st.st_z = r * sin(t) + ZCENTER;
		st.st_lx = (XCENTER - st.st_x) / r * mag;
		st.st_lz = (ZCENTER - st.st_z) / r * mag;
	}
	if (dv != 0 && spkey & GLUT_ACTIVE_SHIFT) {
		adj = (dv < 0) ? 0.005f : -0.005f;
		t = asinf(st.st_ly);
		if (fabs(t + adj) < PI / 2.0f) {
			st.st_ly = sin(t + adj);
			mag = sqrt(SQUARE(st.st_lx) + SQUARE(st.st_ly) +
			    SQUARE(st.st_lz));
			st.st_lx /= mag;
			st.st_ly /= mag;
			st.st_lz /= mag;
		}
	}

	if (st.st_opts & OP_TWEEN) {
		tx = st.st_x;  st.st_x = sx;
		ty = st.st_y;  st.st_y = sy;
		tz = st.st_z;  st.st_z = sz;

		tlx = st.st_lx;  st.st_lx = slx;
		tly = st.st_ly;  st.st_ly = sly;
		tlz = st.st_lz;  st.st_lz = slz;
	} else
		adjcam();
}

void
passive_m(int u, int v)
{
	lastu = u;
	lastv = v;

	detect_node(u, v);
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
			fps = tcnt / (tv.tv_sec - lastsync.tv_sec);
		if (lastsync.tv_sec + SLEEP_INTV < tv.tv_sec) {
			tcnt = 0;
			lastsync.tv_sec = tv.tv_sec;
			rebuild(RO_RELOAD | RO_COMPILE);
		}
		cnt = 0;
	}
	draw();
}

void
make_cluster(void)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	int r, cb, cg, m, n;
	struct node *node;

	if (cluster_dl)
		glDeleteLists(cluster_dl, 1);
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
	for (r = 0; r < NROWS; r++, z += ROWDEPTH + ROWSPACE) {
		for (cb = 0; cb < NCABS; cb++, x += CABWIDTH + CABSPACE) {
			for (cg = 0; cg < NCAGES; cg++, y += CAGEHEIGHT + CAGESPACE) {
				for (m = 0; m < NMODS; m++, x += MODWIDTH + MODSPACE) {
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_pos.np_x = x + NODESPACE;
						node->n_pos.np_y = y +
						    (2.0f * NODESPACE + NODEHEIGHT) *
						    (n % (NNODES/2)) + NODESPACE,
						node->n_pos.np_z = z +
						    (2.0f * NODESPACE + NODEDEPTH) *
						    (n / (NNODES/2)) +
						    (NODESPACE * (1 + n % (NNODES/2))),

						draw_node(node, NODEWIDTH,
						    NODEHEIGHT, NODEDEPTH);
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
	for (i = 0; i < NJST; i++) {
		snprintf(path, sizeof(path), _PATH_TEX, i);
		data = load_png(path);
		load_texture(data, st.st_alpha_fmt, i + 1);
		jstates[i].js_fill.f_texid = i + 1;
	}
}

void
del_textures(void)
{
	int i;

	/* Delete textures from vid memory */
	for (i = 0; i < NJST; i++)
		if (jstates[i].js_fill.f_texid)
			glDeleteTextures(1, &jstates[i].js_fill.f_texid);
}

void
rebuild(int opts)
{
	if (opts & RO_TEX) {
		del_textures();
		load_textures();
	}
	if (opts & RO_PHYS)
		parse_physmap();
	if (opts & RO_RELOAD)
		switch (st.st_mode) {
		case SM_JOBS:
			parse_jobmap();
			if (st.st_selnode != NULL) {
				if (st.st_selnode->n_state != JST_INFO)
					st.st_selnode->n_savst =
					    st.st_selnode->n_state;
				st.st_selnode->n_state = JST_INFO;
			}
			break;
		case SM_FAIL:
			parse_failmap();
			break;
		case SM_TEMP:
			parse_tempmap();
			break;
		}
	if (opts & RO_COMPILE)
		make_cluster();
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

	TAILQ_INIT(&panels);
	buf_init(&uinp.uinp_buf);
	buf_append(&uinp.uinp_buf, '\0');
	rebuild(RO_ALL);

	/* glutExposeFunc(reshape); */
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyh_default);
	glutSpecialFunc(spkeyh_default);
	glutDisplayFunc(draw);
	glutIdleFunc(idle);
	glutMouseFunc(mouse);
	glutMotionFunc(active_m);
	glutPassiveMotionFunc(passive_m);
	glutMainLoop();
	exit(0);
}
