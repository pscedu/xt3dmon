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
#include "buf.h"

extern double round(double); /* don't ask */

#define SLEEP_INTV	5

#define FOVY		(45.0f)
#define ASPECT		(win_width / (double)win_height)

#define STARTX		(-30.0f)
#define STARTY		(10.0f)
#define STARTZ		(25.0f)

#define STARTLX		(0.99f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

#define FPS_TO_USEC(X) (1000000/X)	/* Convert FPS to microseconds. */
#define GOVERN_FPS 20			/* FPS governor. */

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NID_MAX];
int			 win_width = 800;
int			 win_height = 600;
int			 active_flyby = 0;
int			 build_flyby = 0;

int			 logical_width;
int			 logical_height;
int			 logical_depth;
float			 tx = STARTX, tlx = STARTLX, ox = STARTX, olx = STARTLX;
float			 ty = STARTY, tly = STARTLY, oy = STARTY, oly = STARTLY;
float			 tz = STARTZ, tlz = STARTLZ, oz = STARTZ, olz = STARTLZ;
GLint			 cluster_dl;
struct timeval		 lastsync;
long			 fps = 100;
int			 gDebugCapture;

struct vmode {
	int	vm_clip;
} vmodes[] = {
	{ 1000 },
	{ 100 }
};

struct state st = {
	STARTX, STARTY, STARTZ,				/* (x,y,z) */
	STARTLX, STARTLY, STARTLZ,			/* (lx,ly,lz) */
	OP_WIRES | OP_TWEEN | OP_GROUND | OP_DISPLAY,	/* options */
	1.0f,						/* job alpha */
	1.0f,						/* other alpha */
	GL_RGBA,					/* alpha blend format */
	SM_JOBS,					/* which data to show */
	VM_PHYSICAL,					/* viewing mode */
	0						/* rebuild flags */
};

struct job_state jstates[] = {
	{ "Free",		{ 1.00f, 1.00f, 1.00f, 1.00f, 0 } },	/* White */
	{ "Disabled (PBS)",	{ 1.00f, 0.00f, 0.00f, 1.00f, 0 } },	/* Red */
	{ "Disabled (HW)",	{ 0.66f, 0.66f, 0.66f, 1.00f, 0 } },	/* Gray */
	{ NULL,			{ 0.00f, 0.00f, 0.00f, 1.00f, 0 } },	/* (dynamic) */
	{ "Service",		{ 1.00f, 1.00f, 0.00f, 1.00f, 0 } },	/* Yellow */
	{ "Unaccounted",	{ 0.00f, 0.00f, 1.00f, 1.00f, 0 } },	/* Blue */
	{ "Bad",		{ 1.00f, 0.75f, 0.75f, 1.00f, 0 } },	/* Pink */
	{ "Checking",		{ 0.00f, 1.00f, 0.00f, 1.00f, 0 } }	/* Green */
};

struct fill selnodefill = { 0.20f, 0.40f, 0.60f, 1.00f, 0 };		/* Dark blue */
struct node *selnode;

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
	win_width = w;
	win_height = h;

	rebuild(RO_PERSPECTIVE);
	adjcam();
}

struct node *
node_for_nid(int nid)
{
	if (nid > NID_MAX || nid < 0)
		return (NULL);
	return (invmap[nid]);
}

/*
 * Serial entry point to special-case code for handling states changes.
 */
void
refresh_state(int flyby, int oldopts)
{
	int diff = st.st_opts ^ oldopts;

	/* Restore tweening state */
	if (diff & OP_TWEEN) {
		ox = tx = st.st_x;  olx = tlx = st.st_lx;
		oy = ty = st.st_y;  oly = tly = st.st_ly;
		oz = tz = st.st_z;  olz = tlz = st.st_lz;
	}

	if (diff & OP_GOVERN)
		glutIdleFunc(st.st_opts & OP_GOVERN ? idle_govern : idle);

	if (st.st_ro)
		rebuild(st.st_ro);

	if (flyby) {
		st.st_ro = 0;
		flip_panels(fb.fb_panels);
		fb.fb_panels = 0;
	}
}

void
select_node(struct node *n)
{
	if (selnode == n)
		return;
	if (selnode != NULL)
		selnode->n_fillp = selnode->n_ofillp;
	selnode = n;
	if (n->n_fillp != &selnodefill) {
		n->n_ofillp = n->n_fillp;
		n->n_fillp = &selnodefill;
	}
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
	float x = n->n_v->v_x;
	float y = n->n_v->v_y;
	float z = n->n_v->v_z;
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
idle_govern(void)
{
	struct timeval time, diff;
	static struct timeval ftime = {0,0};
	static struct timeval ptime = {0,0};
	static long fcnt = 0;

	gettimeofday(&time, NULL);
	timersub(&time, &ptime, &diff);
	if (diff.tv_sec * 1e9 + diff.tv_usec >= FPS_TO_USEC(GOVERN_FPS)) {
		timersub(&time, &ftime, &diff);
		if (diff.tv_sec >= 1) {
			ftime = time;
			fps = fcnt;
			fcnt = 0;
		}
		timersub(&time, &lastsync, &diff);
		if (diff.tv_sec >= SLEEP_INTV) {
			lastsync = time;
			rebuild(RO_RELOAD | RO_COMPILE);
			/* A lot of time may have elasped. */
//			gettimeofday(&time, NULL);
		}
		ptime = time;
		draw();
		fcnt++;
	}
}

void
idle(void)
{
	static int tcnt, cnt;
printf("hi\n");
	tcnt++;
	if (++cnt >= fps) {
		struct timeval tv, diff;

		if (gettimeofday(&tv, NULL) == -1)
			err(1, "gettimeofday");
		timersub(&tv, &lastsync, &diff);
		if (diff.tv_sec)
			fps = tcnt / (diff.tv_sec + diff.tv_usec / 1e9f);
		if (diff.tv_sec > SLEEP_INTV) {
			tcnt = 0;
			lastsync = tv;
			rebuild(RO_RELOAD | RO_COMPILE);
		}
		cnt = 0;
	}
	draw();
}

__inline void
make_cluster_physical(void)
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
						node->n_physv.v_x = x + NODESPACE;
						node->n_physv.v_y = y +
						    (2.0f * NODESPACE + NODEHEIGHT) *
						    (n % (NNODES/2)) + NODESPACE,
						node->n_physv.v_z = z +
						    (2.0f * NODESPACE + NODEDEPTH) *
						    (n / (NNODES/2)) +
						    (NODESPACE * (1 + n % (NNODES/2))),
						node->n_v = &node->n_physv;
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

__inline void
make_one_logical_cluster(struct vec *v)
{
	int r, cb, cg, m, n;
	struct node *node;

for (r = 0; r < NROWS; r++)
 for (cb = 0; cb < NCABS; cb++)
  for (cg = 0; cg < NCAGES; cg++)
   for (m = 0; m < NMODS; m++)
    for (n = 0; n < NNODES; n++) {
     node = &nodes[r][cb][cg][m][n];
     node->n_v = &node->n_logv;
     node->n_v->v_x += v->v_x;
     node->n_v->v_y += v->v_y;
     node->n_v->v_z += v->v_z;
     draw_node(node, NODEWIDTH, NODEHEIGHT, NODEDEPTH);
     node->n_v->v_x -= v->v_x;
     node->n_v->v_y -= v->v_y;
     node->n_v->v_z -= v->v_z;
    }
}

/*
 * Draw the cluster repeatedly till we reach the clipping plane.
 * Since we can see vm_clip away, we must construct a 3D space
 * vm_clip^3 large and draw the cluster multiple
 * times inside.
 */
__inline void
make_cluster_logical(void)
{
	int clip, xpos, ypos, zpos;
	struct vec v;

	clip = vmodes[VM_LOGICAL].vm_clip;
	xpos = st.st_x - clip;
	ypos = st.st_y - clip;
	zpos = st.st_z - clip;

printf("(%d,%d,%d)\n", xpos, ypos, zpos);

	xpos = SIGN(xpos) * round(abs(xpos) / (double)logical_width) * logical_width;
	ypos = SIGN(ypos) * round(abs(ypos) / (double)logical_height) * logical_height;
	zpos = SIGN(zpos) * round(abs(zpos) / (double)logical_depth) * logical_depth;

	if (cluster_dl)
		glDeleteLists(cluster_dl, 1);
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
	for (v.v_x = xpos; v.v_x < st.st_x + clip; v.v_x += logical_width)
		for (v.v_y = ypos; v.v_y < st.st_y + clip; v.v_y += logical_height)
			for (v.v_z = zpos; v.v_z < st.st_z + clip; v.v_z += logical_depth)
				make_one_logical_cluster(&v);
	glEndList();
}

void
make_cluster(void)
{
	switch (st.st_vmode) {
	case VM_PHYSICAL:
		make_cluster_physical();
		break;
	case VM_LOGICAL:
		make_cluster_logical();
		break;
	}
}

void
load_textures(void)
{
	char path[NAME_MAX];
	void *data;
	int i;

	/* Read in texture IDs. */
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

	/* Delete textures from memory. */
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
	if (opts & RO_RELOAD) {
		switch (st.st_mode) {
		case SM_JOBS:
			parse_jobmap();
			break;
		case SM_FAIL:
			parse_failmap();
			break;
		case SM_TEMP:
			parse_tempmap();
			break;
		}
		if (selnode != NULL) {
			if (selnode->n_fillp != &selnodefill) {
				selnode->n_ofillp = selnode->n_fillp;
				selnode->n_fillp = &selnodefill;
			}
		}
	}
	/* XXX: this is wrong. */
	if (opts & RO_SELNODE) {
		select_node(selnode);
		return;
	}
	if (opts & RO_PERSPECTIVE) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glViewport(0, 0, win_width, win_height);
		gluPerspective(FOVY, ASPECT, 1, vmodes[st.st_vmode].vm_clip);
		glMatrixMode(GL_MODELVIEW);
		adjcam();
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
	glEnable(GL_DEPTH_TEST);

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
	glutMouseFunc(mouseh);
	glutMotionFunc(m_activeh_default);
	glutPassiveMotionFunc(m_passiveh_default);
	glutMainLoop();
	exit(0);
}
