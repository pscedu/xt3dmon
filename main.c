/* $Id$ */

#include <sys/param.h>
#include <sys/time.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <math.h>

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
int			 datasrc = DS_FILE;
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
GLint			 cluster_dl, ground_dl;
struct timeval		 lastsync;
long			 fps = 100;
int			 gDebugCapture;

struct datasrc datasrcsw[] = {
	{ parse_physmap },
	{ db_physmap }
};

struct vmode vmodes[] = {
	{ 1000,	0.2f, 1.2f, 1.2f },
	{ 30,	2.0f, 2.0f, 2.0f },
	{ 1000,	2.0f, 2.0f, 2.0f }
};

struct state st = {
	{ STARTX, STARTY, STARTZ },			/* (x,y,z) */
	{ STARTLX, STARTLY, STARTLZ },			/* (lx,ly,lz) */
	OP_WIRES | OP_TWEEN | OP_GROUND | OP_DISPLAY,	/* options */
	SM_JOBS,					/* which data to show */
	VM_PHYSICAL,					/* viewing mode */
	4,						/* logical node spacing */
	0						/* rebuild flags (unused) */
};

struct job_state jstates[] = {
	{ "Free",		{ 1.00f, 1.00f, 1.00f, 1.00f, 0, 0 } },	/* White */
	{ "Disabled (PBS)",	{ 1.00f, 0.00f, 0.00f, 1.00f, 0, 0 } },	/* Red */
	{ "Disabled (HW)",	{ 0.66f, 0.66f, 0.66f, 1.00f, 0, 0 } },	/* Gray */
	{ NULL,			{ 0.00f, 0.00f, 0.00f, 1.00f, 0, 0 } },	/* (dynamic) */
	{ "Service",		{ 1.00f, 1.00f, 0.00f, 1.00f, 0, 0 } },	/* Yellow */
	{ "Unaccounted",	{ 0.00f, 0.00f, 1.00f, 1.00f, 0, 0 } },	/* Blue */
	{ "Bad",		{ 1.00f, 0.75f, 0.75f, 1.00f, 0, 0 } },	/* Pink */
	{ "Checking",		{ 0.00f, 1.00f, 0.00f, 1.00f, 0, 0 } }	/* Green */
};

struct fill selnodefill = { 0.20f, 0.40f, 0.60f, 1.00f, 0, 0 };		/* Dark blue */
struct node *selnode;

void
reshape(int w, int h)
{
	struct panel *p;

	win_width = w;
	win_height = h;

	rebuild(RO_PERSPECTIVE);
	cam_update();

	TAILQ_FOREACH(p, &panels, p_link)
		p->p_opts |= POPT_DIRTY;
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
refresh_state(int oldopts)
{
	int diff = st.st_opts ^ oldopts;

	/* Restore tweening state. */
	if (diff & OP_TWEEN) {
		ox = tx = st.st_x;  olx = tlx = st.st_lx;
		oy = ty = st.st_y;  oly = tly = st.st_ly;
		oz = tz = st.st_z;  olz = tlz = st.st_lz;
	}
	if (diff & OP_FREELOOK)
		glutMotionFunc(st.st_opts & OP_FREELOOK ?
		    m_activeh_free : m_activeh_default);
	if (diff & OP_GOVERN)
		glutIdleFunc(st.st_opts & OP_GOVERN ? idle_govern : idle);
	if (st.st_ro)
		rebuild(st.st_ro);
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
int
collide(struct vec *box, struct vec *dim,
    struct vec *raystart, struct vec *rayend)
{
	float lo_x, hi_x;
	float lo_y, hi_y, y_sxbox, y_exbox;
	float lo_z, hi_z, z_sxbox, z_exbox;
	float x = box->v_x, y = box->v_y, z = box->v_z;
	float w = dim->v_w, h = dim->v_h, d = dim->v_d;
	float sx = raystart->v_x, sy = raystart->v_y, sz = raystart->v_z;
	float ex = rayend->v_x, ey = rayend->v_y, ez = rayend->v_z;

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
	return (1);
}

/* Node detection */
void
detect_node(int screenu, int screenv)
{
	GLdouble mvm[16], pvm[16], sx, sy, sz, ex, ey, ez;
	GLint x, y, z;
	GLint vp[4];

	struct vec v, dim, raystart, rayend;
	int r, cb, cg, m, c, n;
	float adj;

	/* Grab world info */
	glGetIntegerv(GL_VIEWPORT, vp);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
	glGetDoublev(GL_PROJECTION_MATRIX, pvm);

	/* Fix y coordinate */
	y = vp[3] - screenv - 1;
	x = screenu;

	/* Transform 2d to 3d coordinates according to z */
	z = 0.0;
	if (gluUnProject(x, y, z, mvm, pvm, vp, &sx, &sy, &sz) == GL_FALSE)
		return;

	z = 1.0;
	if (gluUnProject(x, y, z, mvm, pvm, vp, &ex, &ey, &ez) == GL_FALSE)
		return;

	raystart.v_x = sx;
	raystart.v_y = sy;
	raystart.v_z = sz;

	rayend.v_x = ex;
	rayend.v_y = ey;
	rayend.v_z = ez;


#if 0
						n0 = st.st_lz > 0.0f ? n : NNODES - n - 1;
						n1 = st.st_ly > 0.0f ? n : NNODES - n - 1;
						pn = 2 * (n0 / (NNODES / 2)) +
						    n1 % (NNODES / 2);
#endif

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		/* Scan cluster. */
		v.v_x = v.v_y = v.v_z = NODESPACE; /* XXX */
		dim.v_w = ROWWIDTH;
		dim.v_h = CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1);
		dim.v_d = ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1);
		if (!collide(&v, &dim, &raystart, &rayend))
			return;

		/* Scan rows. */
		dim.v_d = ROWDEPTH;
		adj = ROWDEPTH + ROWSPACE;
		if (st.st_lz < 0.0f) {
			adj *= -1.0f;
			v.v_z += ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1) + adj;
		}
		for (r = 0; r < NROWS; r++, v.v_z += adj)
			if (collide(&v, &dim, &raystart, &rayend))
				break;
		if (r == NROWS)
			return;
		if (st.st_lz < 0.0f)
			r = NROWS - r - 1;

		/* Scan cabinets. */
		dim.v_w = CABWIDTH;
		adj = CABWIDTH + CABSPACE;
		if (st.st_lx < 0.0f) {
			adj *= -1.0f;
			v.v_x += ROWWIDTH + adj;
		}
		for (cb = 0; cb < NCABS; cb++, v.v_x += adj)
			if (collide(&v, &dim, &raystart, &rayend))
				break;
		if (cb == NCABS)
			return;
		if (st.st_lx < 0.0f)
			cb = NCABS - cb - 1;

		/* Scan cages. */
		dim.v_h = CAGEHEIGHT;
		adj = CAGEHEIGHT + CAGESPACE;
		if (st.st_ly < 0.0f) {
			adj *= -1.0f;
			v.v_y += CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1) + adj;
		}
		for (cg = 0; cg < NCAGES; cg++, v.v_y += adj)
			if (collide(&v, &dim, &raystart, &rayend))
				break;
		if (cg == NCAGES)
			return;
		if (st.st_ly < 0.0f)
			cg = NCAGES - cg - 1;

		/* Scan modules. */
		dim.v_w = MODWIDTH;
		adj = MODWIDTH + MODSPACE;
		if (st.st_lx < 0.0f) {
			adj *= -1.0f;
			v.v_x += CABWIDTH + adj;
		}
		for (m = 0; m < NMODS; m++, v.v_w += adj)
			if (collide(&v, &dim, &raystart, &rayend))
				break;
		if (m == NMODS)
			return;
		if (st.st_lx < 0.0f)
			m = NMODS - m - 1;
printf("mw: %.4f, nw: %.4f\n", MODWIDTH, NODEWIDTH);
		/* Scan nodes. */
		dim.v_h = NODEHEIGHT;
		dim.v_d = NODEDEPTH;
		for (c = 0; c < NNODES / 2; c++, v.v_z += NODEDEPTH + NODESPACE) {
			for (n = 0; n < NNODES / 2; n++, v.v_y += NODEHEIGHT + NODESPACE)
				if (collide(&v, &dim, &raystart, &rayend))
					goto foundnode;
			v.v_y -= (NODEHEIGHT + NODESPACE) * (NNODES / 2);
		}
		return;
foundnode:
		n += 2 * c;
		select_node(&nodes[r][cb][cg][m][n]);
	}
#if 0
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
						   sx, sy, sz, ex, ey, ez))
							return;
					}
				}
			}
		}
	}
#endif
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
	int r, cb, cg, m, c, n, n0;
	struct node *node;
	struct vec mdim;
	struct fill mf;
	struct vec v;

	if (cluster_dl)
		glDeleteLists(cluster_dl, 1);
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);

	mdim.v_w = MODWIDTH;
	mdim.v_h = MODHEIGHT;
	mdim.v_d = MODDEPTH;

	mf.f_r = 1.00;
	mf.f_g = 1.00;
	mf.f_b = 1.00;
	mf.f_a = 0.30;

	v.v_x = v.v_y = v.v_z = NODESPACE;
	for (r = 0; r < NROWS; r++, v.v_z += ROWDEPTH + ROWSPACE) {
		for (cb = 0; cb < NCABS; cb++, v.v_x += CABWIDTH + CABSPACE) {
			for (cg = 0; cg < NCAGES; cg++, v.v_y += CAGEHEIGHT + CAGESPACE) {
				for (m = 0; m < NMODS; m++, v.v_x += MODWIDTH + MODSPACE) {
					if (st.st_opts & OP_SHOWMODS)
						draw_mod(&v, &mdim, &mf);
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_v = &node->n_physv;
						node->n_physv = v;

						c = (n & 1) ^ ((n & 2) >> 1);
						n0 = n / (NNODES / 2);

						node->n_physv.v_y += n0 * (NODESPACE + NODEHEIGHT);
						node->n_physv.v_z += c * (NODESPACE + NODEDEPTH) +
						    n0 * NODESHIFT;
						draw_node(node);
					}
				}
				v.v_x -= (MODWIDTH + MODSPACE) * NMODS;
			}
			v.v_y -= (CAGEHEIGHT + CAGESPACE) * NCAGES;
		}
		v.v_x -= (CABWIDTH + CABSPACE) * NCABS;
	}
	glEndList();
}

__inline void
make_one_logical_cluster(struct vec *v)
{
	int r, cb, cg, m, n;
	struct node *node;
	struct vec nv;

	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];

						nv.v_x = node->n_logv.v_x * st.st_lognspace + v->v_x;
						nv.v_y = node->n_logv.v_y * st.st_lognspace + v->v_y;
						nv.v_z = node->n_logv.v_z * st.st_lognspace + v->v_z;
						node->n_v = &nv;

						draw_node(node);
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

static int n;
printf("sp: %d\n", st.st_lognspace);
	if (cluster_dl)
		glDeleteLists(cluster_dl, 1);
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);

	clip = vmodes[VM_LOGICAL].vm_clip;
	xpos = st.st_x - clip;
	ypos = st.st_y - clip;
	zpos = st.st_z - clip;

	xpos = SIGN(xpos) * ceil(abs(xpos) / (double)LOGWIDTH) * LOGWIDTH;
	ypos = SIGN(ypos) * ceil(abs(ypos) / (double)LOGHEIGHT) * LOGHEIGHT;
	zpos = SIGN(zpos) * ceil(abs(zpos) / (double)LOGDEPTH) * LOGDEPTH;

	for (v.v_x = xpos; v.v_x < st.st_x + clip; v.v_x += LOGWIDTH)
		for (v.v_y = ypos; v.v_y < st.st_y + clip; v.v_y += LOGHEIGHT)
			for (v.v_z = zpos; v.v_z < st.st_z + clip; v.v_z += LOGDEPTH)
{printf(" %d", ++n);
				make_one_logical_cluster(&v);
}
printf("\n");
n=0;

	glEndList();
}

__inline void
make_cluster_logicalone(void)
{
	struct vec v;

	if (cluster_dl)
		glDeleteLists(cluster_dl, 1);
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);

	v.v_x = v.v_y = v.v_z = 0;
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
	case VM_LOGICALONE:
		make_cluster_logicalone();
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
		load_texture(data, jstates[i].js_fill.f_alpha_fmt, i + 1);
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
		datasrcsw[datasrc].ds_physmap();
	if (opts & RO_RELOAD) {
		mode_data_clean = 0;
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
		cam_update();
	}
	if (opts & RO_INIT)
		make_ground();
	if (opts & RO_COMPILE)
		make_cluster();
}

int
main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "l")) != -1)
		switch (c) {
		case 'l':
			datasrc = DS_DB;
			dbh_connect(&dbh);
			break;
		}

	/* op_reset = 1; */
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
	rebuild(RO_INIT);

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
