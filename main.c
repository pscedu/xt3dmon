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

#include "mon.h"
#include "slist.h"
#include "cdefs.h"

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

#define STARTX		(-30.0f)
#define STARTY		(10.0f)
#define STARTZ		(25.0f)

#define STARTLX		(0.99f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

void			 load_textures(void);
void			 del_textures(void);
void			 make_cluster(void);

struct job		**jobs;
size_t			 njobs;
struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NLOGIDS][NROWS * NCABS * NCAGES * NMODS * NNODES];
struct node		 *selnode;
int			 win_width = 800;
int			 win_height = 600;

int			 active_fps = 0;
int			 active_ninfo = 0;
int			 active_flyby = 0;

float			 tx = STARTX, tlx = STARTLX;
float			 ty = STARTY, tly = STARTLY;
float			 tz = STARTZ, tlz = STARTLZ;
int			 spkey, xpos, ypos;
GLint			 cluster_dl;
struct timeval		 lastsync;
long			 fps;
struct lineseglh	 seglh;

struct state st = {
	STARTX, STARTY, STARTZ,				/* (x,y,z) */
	STARTLX, STARTLY, STARTLZ,			/* (lx,ly,lz) */
	OP_WIRES | OP_TWEEN | OP_GROUND,		/* options */
	0,						/* panels */
	1.0f,						/* job alpha */
	1.0f,						/* other alpha */
	GL_RGBA,					/* alpha blend format */
	0,						/* tween mode (unused) */
	0						/* nframes (unused) */
};

struct nstate nstates[] = {
	{ "Free",		1.00, 1.00, 1.00, 1 },	/* White */
	{ "Disabled (PBS)",	1.00, 0.00, 0.00, 1 },	/* Red */
	{ "Disabled (HW)",	0.66, 0.66, 0.66, 1 },	/* Gray */
	{ NULL,			0.00, 0.00, 0.00, 1 },	/* (dynamic) */
	{ "I/O",		1.00, 1.00, 0.00, 1 },	/* Yellow */
	{ "Unaccounted",	0.00, 0.00, 1.00, 1 },	/* Blue */
	{ "Bad",		1.00, 0.75, 0.75, 1 },	/* Pink */
	{ "Checking",		0.00, 1.00, 0.00, 1 },	/* Green */
	{ "Info",		0.20, 0.40, 0.60, 1 }	/* Dark blue */
};

void
calc_flyby(void)
{
	static const struct state *curst = NULL;
	static struct state lastst;
	float frac;

	if (curst == NULL) {
		curst = flybypath;
		lastst = st;
		lastst.st_nframes = 1;
	} else if (++lastst.st_nframes > curst->st_nframes) {
		if ((++curst)->st_nframes == 0) {
			active_flyby = 0;
			curst = NULL;
			return;
		}
		lastst.st_nframes = 0;
		st.st_opts = curst->st_opts;
		st.st_panels = curst->st_panels;
		st.st_alpha_fmt = curst->st_alpha_fmt;
		st.st_alpha_job = curst->st_alpha_job;
		st.st_alpha_oth = curst->st_alpha_oth;
	}

	frac = (lastst.st_nframes / curst->st_nframes);
	st.st_x = (curst->st_x - lastst.st_x) / frac;
	st.st_y = (curst->st_y - lastst.st_y) / frac;
	st.st_z = (curst->st_z - lastst.st_z) / frac;
	st.st_lx = (curst->st_lx - lastst.st_lx) / frac;
	st.st_ly = (curst->st_ly - lastst.st_ly) / frac;
	st.st_lz = (curst->st_lz - lastst.st_lz) / frac;
}

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

void
key(unsigned char key, __unused int u, __unused int v)
{
	if (active_flyby) {
		if (key == 'F')
			active_flyby = 0;
		return;
	}

	switch (key) {
	case 'b':
		st.st_opts ^= OP_BLEND;
		break;
	case 'c':
		if (selnode != NULL && selnode->n_savst)
			selnode->n_state = selnode->n_savst;
		break;
	case 'd':
		st.st_opts ^= OP_CAPTURE;
	case 'e':
		if (st.st_opts & OP_TWEEN)
			st.st_opts &= ~OP_TWEEN;
		else {
			st.st_opts |= OP_TWEEN;
			tx = st.st_x;  tlx = st.st_lx;
			ty = st.st_y;  tly = st.st_ly;
			tz = st.st_z;  tlz = st.st_lz;
		}
		break;
	case 'F':
		active_flyby = !active_flyby;
		break;
	case 'f':
		st.st_panels ^= PANEL_FPS;
		active_fps = 1;
		if (st.st_panels & PANEL_NINFO)
			active_ninfo = 1;
		break;
	case 'g':
		st.st_opts ^= OP_GROUND;
		break;
	case 'l': {
		struct lineseg *ln, *pln;

		if (st.st_opts & OP_LINELEAVE)
			st.st_opts = (st.st_opts & ~OP_LINELEAVE) | OP_LINEFOLLOW;
		else if (st.st_opts & OP_LINEFOLLOW)
			st.st_opts &= ~OP_LINEFOLLOW;
		else
			st.st_opts = OP_LINELEAVE;

		for (ln = SLIST_FIRST(&seglh); ln != NULL; ln = pln) {
			pln = SLIST_NEXT(ln, ln_next);
			free(ln);
		}
		SLIST_INIT(&seglh);
		break;
	    }
	case 'n':
		st.st_panels ^= PANEL_NINFO;
		active_ninfo = 1;
		break;
	case 'p':
		printf("pos[%.2f,%.2f,%.2f] look[%.2f,%.2f,%.2f]\n",
		    st.st_x, st.st_y, st.st_z,
		    st.st_lx, st.st_ly, st.st_lz);
		break;
	case 'q':
		exit(0);
		/* NOTREACHED */
	case 't':
		st.st_opts ^= OP_TEX;
		break;
	case 'T':
		st.st_alpha_fmt = (st.st_alpha_fmt == GL_RGBA ? GL_INTENSITY : GL_RGBA);
		del_textures();
		load_textures();
		break;
	case 'w':
		st.st_opts ^= OP_WIRES;
		break;
	case '+':
		st.st_alpha_job += (st.st_alpha_job + TRANS_INC > 1.0 ? 0.0 : TRANS_INC);
		break;
	case '_':
		st.st_alpha_job -= (st.st_alpha_job + TRANS_INC < 0.0 ? 0.0 : TRANS_INC);
		break;
	case '=':
		st.st_alpha_oth += (st.st_alpha_oth + TRANS_INC > 1.0 ? 0.0 : TRANS_INC);
		break;
	case '-':
		st.st_alpha_oth -= (st.st_alpha_oth + TRANS_INC < 0.0 ? 0.0 : TRANS_INC);
		break;
	}

	/* resync */
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
sp_key(int key, __unused int u, __unused int v)
{
	float sx, sy, sz;
	float r, adj;

	if (active_flyby)
		return;

	r = sqrt(SQUARE(st.st_x - XCENTER) + SQUARE(st.st_z - ZCENTER));
	adj = pow(2, r / (ROWWIDTH / 2.0f));
	if (adj > 50.0f)
		adj = 50.0f;

	sx = sy = sz = 0.0f; /* gcc */
	if (st.st_opts & OP_TWEEN) {
		sx = st.st_x;  st.st_x = tx;
		sy = st.st_y;  st.st_y = ty;
		sz = st.st_z;  st.st_z = tz;
	}

	switch (key) {
	case GLUT_KEY_LEFT:
		st.st_x += st.st_lz * 0.3f * SCALE * adj;
		st.st_z -= st.st_lx * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_RIGHT:
		st.st_x -= st.st_lz * 0.3f * SCALE * adj;
		st.st_z += st.st_lx * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_UP:			/* Forward */
		st.st_x += st.st_lx * 0.3f * SCALE * adj;
		st.st_y += st.st_ly * 0.3f * SCALE * adj;
		st.st_z += st.st_lz * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_DOWN:			/* Backward */
		st.st_x -= st.st_lx * 0.3f * SCALE * adj;
		st.st_y -= st.st_ly * 0.3f * SCALE * adj;
		st.st_z -= st.st_lz * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_PAGE_UP:
		st.st_x += st.st_lx * st.st_ly * 0.3f * SCALE * adj;
		st.st_y += 0.3f * SCALE;
		st.st_z += st.st_lz * st.st_ly * 0.3f * SCALE * adj;
		break;
	case GLUT_KEY_PAGE_DOWN:
		st.st_x -= st.st_lx * st.st_ly * 0.3f * SCALE * adj;
		st.st_y -= 0.3f * SCALE;
		st.st_z -= st.st_lz * st.st_ly * 0.3f * SCALE * adj;
		break;
	default:
		return;
	}

	if (st.st_opts & OP_TWEEN) {
		tx = st.st_x;  st.st_x = sx;
		ty = st.st_y;  st.st_y = sy;
		tz = st.st_z;  st.st_z = sz;
	} else
		adjcam();
}

void
select_node(struct node *n)
{
	if (selnode == n)
		return;
	if (selnode != NULL)
		selnode->n_state = selnode->n_savst;
	selnode = n;
	if (n->n_state != ST_INFO)
		n->n_savst = n->n_state;
	n->n_state = ST_INFO;
	make_cluster();
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

/*
 * Node detection
 */
void
detect_node(int u, int v)
{
	int r, cb, cg, m, n;
	int pr, pcb, pcg, pm;
	int n0, n1, pn;

	float rad = sqrt(SQUARE(st.st_x - XCENTER) + SQUARE(st.st_z - ZCENTER));
	float dist = rad + ROWWIDTH;

	float adju = FOVY * ASPECT * 2.0f * PI / 360.0f *
	    (u - win_width / 2.0f) / win_width;
	float angleu = acosf(st.st_lx);
	if (st.st_lz < 0)
		angleu = 2.0f * PI - angleu;
	float dx = cos(angleu + adju);
	float dz = sin(angleu + adju);

	float adjv = FOVY * 2.0f * PI / 360.0f *
	    (win_height / 2.0f - v) / win_height;
	float anglev = asinf(st.st_ly);
	float dy = sin(anglev + adjv);

	dx = st.st_x + dist * dx;
	dy = st.st_y + dist * dy;
	dz = st.st_z + dist * dz;

	if (st.st_opts & (OP_LINELEAVE | OP_LINEFOLLOW)) {
		struct lineseg *ln;

		if (st.st_opts & OP_LINEFOLLOW || SLIST_EMPTY(&seglh)) {
			if ((ln = malloc(sizeof(*ln))) == NULL)
				err(1, NULL);
			SLIST_INSERT_HEAD(&seglh, ln, ln_next);
		} else
			ln = SLIST_FIRST(&seglh);
		ln->ln_sx = st.st_x + 1.0f * st.st_lx;  ln->ln_ex = dx;
		ln->ln_sy = st.st_y + 1.0f * st.st_ly;  ln->ln_ey = dy;
		ln->ln_sz = st.st_z + 1.0f * st.st_lz;  ln->ln_ez = dz;
	}

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
						    st.st_x, st.st_y, st.st_z, dx, dy, dz))
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

	spkey = glutGetModifiers();
	xpos = u;
	ypos = v;
}

void
active_m(int u, int v)
{
	int du = u - xpos, dv = v - ypos;
	float sx, sy, sz, slx, sly, slz;
	float adj, t, r, mag;

	if (active_flyby)
		return;

	xpos = u;
	ypos = v;

	sx = sy = sz = 0.0f; /* gcc */
	slx = sly = slz = 0.0f; /* gcc */
	if (st.st_opts & OP_TWEEN) {
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
	xpos = u;
	ypos = v;

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
			parse_jobmap();
			make_cluster();
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
	for (i = 0; i < NST; i++) {
		snprintf(path, sizeof(path), _PATH_TEX, i);
		data = load_png(path);
		load_texture(data, st.st_alpha_fmt, i + 1);
		nstates[i].nst_texid = i + 1;
	}
}

void
del_textures(void)
{
	int i;

	/* Delete textures from vid memory */
	for (i = 0; i < NST; i++)
		glDeleteTextures(1, &nstates[i].nst_texid);
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
