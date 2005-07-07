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

#define SLEEP_INTV	500

#define STARTX		(-30.0f)
#define STARTY		(10.0f)
#define STARTZ		(25.0f)

#define STARTLX		(0.99f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

#define FPS_TO_USEC(X)	(1000000/X)	/* Convert FPS to microseconds. */
#define GOVERN_FPS	15		/* FPS governor. */

#define SELNODE_GAP	(0.1f)

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NID_MAX];
int			 datasrc = DS_FILE;
int			 win_width = 800;
int			 win_height = 600;
int			 active_flyby = 0;
int			 build_flyby = 0;
int			 capture_mode = PNG_FRAMES;
int			 font_id;
int			 wired_width;
int			 wired_height;
int			 wired_depth;
float			 tx = STARTX, tlx = STARTLX, ox = STARTX, olx = STARTLX;
float			 ty = STARTY, tly = STARTLY, oy = STARTY, oly = STARTLY;
float			 tz = STARTZ, tlz = STARTLZ, oz = STARTZ, olz = STARTLZ;
GLint			 cluster_dl, ground_dl, select_dl;
struct timeval		 lastsync;
long			 fps = 50;
int			 gDebugCapture;

const char *opdesc[] = {
	"Texture mode",
	"Blending mode",
	"Node wireframes",
	"Ground and axes",
	"Camera tweening",
	"Capture mode",
	"Display mode",
	"Govern mode",
	"Flyby loop mode",
	"Debug mode",
	"Free look mode",
	"Node labels",
	"Module mode",
	"Wired cluster frames",
	"Pipe mode",
	"Selected node pipe mode",
	"Non-select-node dimming"
};

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
	VM_PHYSICAL,						/* viewing mode */
	4,							/* wired node spacing */
	0							/* rebuild flags (unused) */
};

struct job_state jstates[] = {
	{ "Free",		{ 1.00f, 1.00f, 1.00f, 1.00f, 0, GL_INTENSITY} },	/* White */
	{ "Disabled (PBS)",	{ 1.00f, 0.00f, 0.00f, 1.00f, 0, GL_INTENSITY} },	/* Red */
	{ "Disabled (HW)",	{ 0.66f, 0.66f, 0.66f, 1.00f, 0, GL_INTENSITY} },	/* Gray */
	{ NULL,			{ 0.00f, 0.00f, 0.00f, 1.00f, 0, GL_INTENSITY} },	/* (dynamic) */
	{ "Service",		{ 1.00f, 1.00f, 0.00f, 1.00f, 0, GL_INTENSITY} },	/* Yellow */
	{ "Unaccounted",	{ 0.00f, 0.00f, 1.00f, 1.00f, 0, GL_INTENSITY} },	/* Blue */
	{ "Bad",		{ 1.00f, 0.75f, 0.75f, 1.00f, 0, GL_INTENSITY} },	/* Pink */
	{ "Checking",		{ 0.00f, 1.00f, 0.00f, 1.00f, 0, GL_INTENSITY} }	/* Green */
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
	int dup, i;

	dup = diff;
	while (dup) {
		i = ffs(dup) - 1;
		dup &= ~(1 << i);
		panel_status_addinfo("%s %s", opdesc[i],
		    (st.st_opts & (1 << i) ? "enabled\n" : "disabled\n"));
	}

	/* Restore tweening state. */
	if (diff & OP_TWEEN) {
		ox = tx = st.st_x;  olx = tlx = st.st_lx;
		oy = ty = st.st_y;  oly = tly = st.st_ly;
		oz = tz = st.st_z;  olz = tlz = st.st_lz;
	}
	if (diff & (OP_BLEND | OP_TEX))
		restore_textures();
	if (diff & OP_FREELOOK)
		glutMotionFunc(st.st_opts & OP_FREELOOK ?
		    m_activeh_free : m_activeh_default);
	if (diff & OP_GOVERN)
		glutIdleFunc(st.st_opts & OP_GOVERN ? idle_govern : idle);
	if (st.st_rf) {
		rebuild(st.st_rf);
		st.st_rf = 0;
	}
}

void
select_node(struct node *n)
{
	if (selnode == n)
		return;
	selnode = n;

	if (select_dl)
		glDeleteLists(select_dl, 1);
	if (n == NULL) {
		select_dl = 0;
		panel_hide(PANEL_NINFO);
		return;
	}
	panel_show(PANEL_NINFO);

	select_dl = glGenLists(1);
	glNewList(select_dl, GL_COMPILE);

	if (n->n_fillp != &selnodefill) {	/* XXX:  is this test needed? */
		n->n_ofillp = n->n_fillp;
		n->n_fillp = &selnodefill;
	}

	n->n_v->v_x -= SELNODE_GAP;
	n->n_v->v_y -= SELNODE_GAP;
	n->n_v->v_z -= SELNODE_GAP;

	vmodes[st.st_vmode].vm_ndim.v_w += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.v_h += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.v_d += SELNODE_GAP * 2;

	glPushMatrix();
	glPushName(n->n_nid);
	glTranslatef(n->n_v->v_x, n->n_v->v_y, n->n_v->v_z);
	if (st.st_opts & OP_SELPIPES && (st.st_opts & OP_PIPES) == 0)
		draw_node_pipes(&vmodes[st.st_vmode].vm_ndim);

	draw_node(n, NDF_DONTPUSH);

	n->n_fillp = n->n_ofillp;

	vmodes[st.st_vmode].vm_ndim.v_w -= SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.v_h -= SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.v_d -= SELNODE_GAP * 2;

	glPopName();
	glPopMatrix();

	n->n_v->v_x += SELNODE_GAP;
	n->n_v->v_y += SELNODE_GAP;
	n->n_v->v_z += SELNODE_GAP;

	glEndList();
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
		if (diff.tv_sec) {
			fps = tcnt / (diff.tv_sec + diff.tv_usec / 1e9f);
			panel_status_setinfo("");
		}
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

	/* Load the font texture */
	font_id = i + 1;
	data = load_png(_PATH_FONT);
	load_texture(data, GL_RGBA, font_id);
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
restore_textures(void)
{
	/* Reload the textures if needed */
	if(st.st_opts & OP_TEX) {
		int fmt = jstates[0].js_fill.f_alpha_fmt;
		if((st.st_opts & OP_BLEND && fmt != GL_INTENSITY) ||
		   (!(st.st_opts & OP_BLEND) && fmt != GL_RGBA))
			update_textures();
	}
}

void
update_textures(void)
{
	int j, newfmt;

	newfmt = (jstates[0].js_fill.f_alpha_fmt == GL_RGBA ?
	    GL_INTENSITY : GL_RGBA);
	for (j = 0; j < NJST; j++)
		jstates[j].js_fill.f_alpha_fmt = newfmt;
	st.st_rf |= RF_TEX | RF_CLUSTER;
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
			obj_batch_start((void ***)&jobs, njobs);
			parse_jobmap();
			obj_batch_end((void ***)&jobs, &njobs);
			break;
		case SM_FAIL:
			obj_batch_start((void ***)&fails, nfails);
			parse_failmap();
			obj_batch_end((void ***)&fails, &nfails);
			break;
		case SM_TEMP:
			obj_batch_start((void ***)&temps, ntemps);
			parse_tempmap();
			obj_batch_end((void ***)&temps, &ntemps);
			break;
		}
	}
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
	if (opts & RF_SELNODE) {
		struct node *n;

		/*
		 * Hack around optimization of not rebuilding
		 * the selected node when it is already selected.
		 */
		n = selnode;
		selnode = NULL;
		select_node(n);
	}
}

int
main(int argc, char *argv[])
{
	int c;

	glutInit(&argc, argv);

	while ((c = getopt(argc, argv, "l")) != -1)
		switch (c) {
		case 'l':
			datasrc = DS_DB;
			dbh_connect(&dbh);
			break;
		}

	/* op_reset = 1; */
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
