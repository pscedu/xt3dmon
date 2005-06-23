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

#define SLEEP_INTV	500

#define FOVY		(45.0f)
#define ASPECT		(win_width / (double)win_height)

#define STARTX		(-30.0f)
#define STARTY		(10.0f)
#define STARTZ		(25.0f)

#define STARTLX		(0.99f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

#define FPS_TO_USEC(X) (1000000/X)	/* Convert FPS to microseconds. */
#define GOVERN_FPS 15			/* FPS governor. */

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NID_MAX];
int			 datasrc = DS_FILE;
int			 win_width = 800;
int			 win_height = 600;
int			 active_flyby = 0;
int			 build_flyby = 0;

int			 wired_width;
int			 wired_height;
int			 wired_depth;
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
	{ 1000,	{ 0.2f, 1.2f, 1.2f } },
	{ 5,	{ 0.5f, 0.5f, 0.5f } },
	{ 1000,	{ 2.0f, 2.0f, 2.0f } }
};

struct state st = {
	{ STARTX, STARTY, STARTZ },				/* (x,y,z) */
	{ STARTLX, STARTLY, STARTLZ },				/* (lx,ly,lz) */
	OP_WIREFRAME | OP_TWEEN | OP_GROUND | OP_DISPLAY,	/* options */
	SM_JOBS,						/* which data to show */
	VM_WIRED,						/* viewing mode */
	4,							/* wired node spacing */
	0							/* rebuild flags (unused) */
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

	rebuild(RF_PERSPECTIVE);
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
	if (st.st_rf)
		rebuild(st.st_rf);
		/* st.st_rf = 0; */
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
	rebuild(RF_CLUSTER);
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

void
phys_detect(struct vec *rs, struct vec *re)
{
	float adj, cbadj, cgadj, madj, sadjy, sadjz, nadj;
	int r, cb, cg, m, s, n0, n;
	struct vec v, d;

	/* Scan cluster. */
	v.v_x = v.v_y = v.v_z = NODESPACE; /* XXX */
	d.v_w = ROWWIDTH;
	d.v_h = CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1);
	d.v_d = ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1);
	if (!collide(&v, &d, rs, re))
		return;

	/* Scan rows. */
	d.v_d = ROWDEPTH;
	adj = ROWDEPTH + ROWSPACE;
	if (st.st_lz < 0.0f) {
		v.v_z += ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1) - ROWDEPTH;
		adj *= -1.0f;
	}
	for (r = 0; r < NROWS; r++, v.v_z += adj) {
		if (collide(&v, &d, rs, re)) {
			struct vec cb_v = v, cb_d = d;

			/* Scan cabinets. */
			cb_d.v_w = CABWIDTH;
			cbadj = CABWIDTH + CABSPACE;
			if (st.st_lx < 0.0f) {
				cb_v.v_x += ROWWIDTH - CABWIDTH;
				cbadj *= -1.0f;
			}
			for (cb = 0; cb < NCABS; cb++, cb_v.v_x += cbadj)
				if (collide(&cb_v, &cb_d, rs, re)) {
					struct vec cg_v = cb_v, cg_d = cb_d;

					/* Scan cages. */
					cg_d.v_h = CAGEHEIGHT;
					cgadj = CAGEHEIGHT + CAGESPACE;
					if (st.st_ly < 0.0f) {
						cg_v.v_y += CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1) - CAGEHEIGHT;
						cgadj *= -1.0f;
					}
					for (cg = 0; cg < NCAGES; cg++, cg_v.v_y += cgadj)
						if (collide(&cg_v, &cg_d, rs, re)) {
							struct vec m_v = cg_v, m_d = cg_d;

							/* Scan modules. */
							m_d.v_w = MODWIDTH;
							madj = MODWIDTH + MODSPACE;
							if (st.st_lx < 0.0f) {
								m_v.v_x += CABWIDTH - MODWIDTH;
								madj *= -1.0f;
							}
							for (m = 0; m < NMODS; m++, m_v.v_w += madj)
								if (collide(&m_v, &m_d, rs, re)) {
									struct vec s_v = m_v, s_d = m_d;

									/* Scan strips. */
									s_d.v_h = NODEHEIGHT;
									sadjy = NODEHEIGHT + NODESPACE;
									sadjz = NODESHIFT;
									if (st.st_ly < 0.0f) {
										s_v.v_y += MODHEIGHT - NODEHEIGHT;
										s_v.v_z += sadjz;
										sadjy *= -1.0f;
										sadjz *= -1.0f;
									}
									for (s = 0; s < NNODES / 2; s++, s_v.v_y += sadjy, s_v.v_z += sadjz)
										if (collide(&s_v, &s_d, rs, re)) {
											struct vec n_v = s_v, n_d = s_d;

											/* Scan nodes. */
											n_d.v_d = NODEDEPTH;
											nadj = NODEDEPTH + NODESPACE;
											if (st.st_lz < 0.0f) {
												n_v.v_z += NODESPACE;
												nadj *= -1.0f;
											}
											for (n0 = 0; n0 < NNODES / 2; n0++, n_v.v_z += nadj)
												if (collide(&n_v, &n_d, rs, re)) {
													if (st.st_lz < 0.0f)
														r = NROWS - r - 1;
													if (st.st_lx < 0.0f)
														cb = NCABS - cb - 1;
													if (st.st_ly < 0.0f)
														cg = NCAGES - cg - 1;
													if (st.st_lx < 0.0f)
														m = NMODS - m - 1;
													if (st.st_ly < 0.0f)
														s = NNODES / 2 - s - 1;
													if (st.st_lz < 0.0f)
														n0 = NNODES / 2 - n0 - 1;

													n = (s << 1) + (s ^ n0);
{ struct node *cn = selnode;
													select_node(&nodes[r][cb][cg][m][n]);
if (cn != selnode) {
  printf("s: %d, n0: %d, n: %d\r", s, n0, n);
  fflush(stdout);
 }
}
													return;
												}
										}
								}
						}
				}
		}
	}
}


/* Node detection */
void
detect_node(int screenu, int screenv)
{
	GLdouble mvm[16], pvm[16], sx, sy, sz, ex, ey, ez;
	GLint x, y, z;
	GLint vp[4];

	struct vec raystart, rayend;

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

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		phys_detect(&raystart, &rayend);
		break;
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
			rebuild(RF_DATASRC | RF_CLUSTER);
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
			rebuild(RF_DATASRC | RF_CLUSTER);
		}
		cnt = 0;
	}
	draw();
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
	if (opts & RF_TEX) {
		del_textures();
		load_textures();
	}
	if (opts & RF_PHYSMAP)
		datasrcsw[datasrc].ds_physmap();
	if (opts & RF_DATASRC) {
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
	if (opts & RF_SELNODE)
		select_node(selnode);
	if (opts & RF_PERSPECTIVE) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glViewport(0, 0, win_width, win_height);
		gluPerspective(FOVY, ASPECT, 1, WI_CLIP);
		glMatrixMode(GL_MODELVIEW);
		cam_update();
	}
	if (opts & RF_GROUND && st.st_opts & OP_GROUND)
		make_ground();
	if (opts & RF_CLUSTER)
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
	rebuild(RF_INIT);

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
