/* $Id$ */

#include "compat.h"

#include <math.h>
#include <time.h>

#include "cdefs.h"
#include "mon.h"

#define SLEEP_INTV	5

#define STARTX		(-30.0f)
#define STARTY		(10.0f)
#define STARTZ		(25.0f)

#define STARTLX		(0.99f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

#define FPS_TO_USEC(x)	(1e6 / x)	/* Convert FPS to microseconds. */
#define GOVERN_FPS	30		/* FPS governor. */

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NID_MAX];
struct node		*wimap[WIDIM_WIDTH][WIDIM_HEIGHT][WIDIM_DEPTH];
int			 dsp = DSP_LOCAL;
int			 win_width = 800;
int			 win_height = 600;
int			 flyby_mode = FBM_OFF;
int			 capture_mode = CM_PNG;
int			 stereo_mode;
int			 font_id;
struct fvec		 tv = { STARTX, STARTY, STARTZ };
struct fvec		 tlv = { STARTLX, STARTLY, STARTLZ };
struct fvec		 tuv = { 0.0f, 1.0f, 0.0f };
GLint			 cluster_dl, ground_dl, select_dl;
struct timeval		 lastsync;
long			 fps = 50;
void			(*drawh)(void);
int			 window_ids[2];
const char		*progname;

const char *opdesc[] = {
	/*  0 */ "Texture mode",
	/*  1 */ "Blending mode",
	/*  2 */ "Node wireframes",
	/*  3 */ "Ground and axes",
	/*  4 */ "Camera tweening",
	/*  5 */ "Capture mode",
	/*  6 */ "Display mode",
	/*  7 */ "Govern mode",
	/*  8 */ "Flyby loop mode",
	/*  9 */ "Debug mode",
	/* 10 */ "Node labels",
	/* 11 */ "Module mode",
	/* 12 */ "Wired cluster frames",
	/* 13 */ "Pipe mode",
	/* 14 */ "Selected node pipe mode",
	/* 15 */ "Pause",
	/* 16 */ "Job tour mode",
	/* 17 */ "Skeleton"
};

struct vmode vmodes[] = {
	{ 1000,	{ 0.2f, 1.2f, 1.2f } },
	{ 5,	{ 0.5f, 0.5f, 0.5f } },
	{ 1000,	{ 2.0f, 2.0f, 2.0f } }
};

struct state st = {
	{ STARTX, STARTY, STARTZ },				/* (x,y,z) */
	{ STARTLX, STARTLY, STARTLZ },				/* (lx,ly,lz) */
	{ 0.0f, 1.0f, 0.0f },					/* (ux,uy,uz) */
	OP_WIREFRAME | OP_TWEEN | OP_GROUND | OP_DISPLAY,	/* options */
	SM_JOBS,						/* which data to show */
	VM_PHYSICAL,						/* viewing mode */
	{ 4, 4, 4 },						/* wired node spacing */
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

void
resizeh(int w, int h)
{
	struct panel *p;

	win_width = w;
	win_height = h;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, win_width, win_height);
	gluPerspective(FOVY, ASPECT, 1, clip);
	glMatrixMode(GL_MODELVIEW);
	cam_dirty = 1;

	TAILQ_FOREACH(p, &panels, p_link)
		p->p_opts |= POPT_DIRTY;
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
		status_add("%s %s", opdesc[i],
		    (st.st_opts & (1 << i) ? "enabled\n" : "disabled\n"));
	}

	/* Restore tweening state. */
	if (diff & OP_TWEEN) {
		tv.fv_x = st.st_x;  tlv.fv_x = st.st_lx;
		tv.fv_y = st.st_y;  tlv.fv_y = st.st_ly;
		tv.fv_z = st.st_z;  tlv.fv_z = st.st_lz;
	}
	if (diff & (OP_BLEND | OP_TEX))
		tex_restore();
	if (diff & OP_GOVERN)
		glutIdleFunc(st.st_opts & OP_GOVERN ?
		    idleh_govern : idleh_default);
}

void
idleh_govern(void)
{
	static struct timeval gov_tv, fps_tv, tv, diff;
	static long fcnt;

	gettimeofday(&tv, NULL);
	timersub(&tv, &gov_tv, &diff);
	if (diff.tv_sec * 1e6 + diff.tv_usec >= FPS_TO_USEC(GOVERN_FPS)) {
		gov_tv = tv;

		timersub(&tv, &fps_tv, &diff);
		if (diff.tv_sec) {
			fps_tv = tv;
			fps = fcnt;
			fcnt = 0;
		}

		timersub(&tv, &lastsync, &diff);
		if (diff.tv_sec >= SLEEP_INTV) {
			lastsync = tv;
			st.st_rf |= RF_DATASRC | RF_CLUSTER;
			status_clear();
		}

		(*drawh)();
		fcnt++;
	}
}

void
idleh_default(void)
{
	static struct timeval tv, diff, fps_tv;
	static int tcnt, cnt;

	tcnt++;
	if (++cnt >= fps + 1) {
		if (gettimeofday(&tv, NULL) == -1)
			err(1, "gettimeofday");

		timersub(&tv, &fps_tv, &diff);
		if (diff.tv_sec) {
			fps = tcnt / (diff.tv_sec + diff.tv_usec / 1e6f);
			fps_tv = tv;
			tcnt = 0;
		}

		timersub(&tv, &lastsync, &diff);
		if (diff.tv_sec > SLEEP_INTV) {
			lastsync = tv;
			st.st_rf |= RF_DATASRC | RF_CLUSTER;
			status_clear();
		}
		cnt = 0;
	}
	(*drawh)();
}

void
rebuild(int opts)
{
	if (opts & RF_TEX) {
		tex_remove();
		tex_load();
	}
	if (opts & RF_PHYSMAP)
		ds_refresh(DS_PHYS, 0);
	if (opts & RF_DATASRC) {
		mode_data_clean = 0;
		switch (st.st_mode) {
		case SM_JOBS:
			obj_batch_start(&job_list);
			parse_jobmap();
			obj_batch_end(&job_list);
			break;
		case SM_FAIL:
			obj_batch_start(&fail_list);
			parse_failmap();
			obj_batch_end(&fail_list);
			break;
		case SM_TEMP:
			obj_batch_start(&temp_list);
			parse_tempmap();
			obj_batch_end(&temp_list);
			break;
		}
		parse_mem();
	}
	if (opts & RF_CAM) {
		switch (st.st_vmode) {
		case VM_PHYSICAL:
		case VM_WIREDONE:
			clip = vmodes[st.st_vmode].vm_clip;
			break;
		case VM_WIRED:
			clip = MIN3(WIV_CLIPX, WIV_CLIPY, WIV_CLIPZ);
			break;
		}
		cam_dirty = 1;
	}
	if (opts & RF_GROUND && st.st_opts & OP_GROUND)
		make_ground();
	if (opts & RF_CLUSTER)
		make_cluster();
	if (opts & RF_SELNODE)
		make_select();
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-alp]\n", progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int flags, c, sw, sh;
	int server = 0;

	progname = argv[0];
	drawh = drawh_default;

	flags = GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE;
	glutInit(&argc, argv);
	while ((c = getopt(argc, argv, "adlp")) != -1)
		switch (c) {
		case 'a':
			flags |= GLUT_STEREO;
			drawh = drawh_stereo;
			stereo_mode = STM_ACT;
			break;
		case 'd':
			server = 1;
			break;
		case 'l':
			dsp = DSP_DB;
			break;
		case 'p':
			drawh = drawh_stereo;
			stereo_mode = STM_PASV;
			break;
		default:
			usage();
			/* NOTREACHED */
		}

	/* XXX:  Sanity-check flags. */
	if (server)
		serv_init();
	if (dsp == DSP_DB)
		dbh_connect(&dbh);

	glutInitDisplayMode(flags);
	sw = glutGet(GLUT_SCREEN_WIDTH);
	sh = glutGet(GLUT_SCREEN_HEIGHT);
	if (stereo_mode == STM_PASV)
		sw /= 2;
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(sw, sh);
	if ((window_ids[0] = glutCreateWindow("XT3 Monitor")) == GL_FALSE)
		errx(1, "glutCreateWindow");
	if (stereo_mode == STM_PASV) {
		glutInitWindowPosition(sw, 0);
		if ((window_ids[1] = glutCreateWindow("XT3 Monitor")) ==
		    GL_FALSE)
			errx(1, "glutCreateWindow");
	}

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	/* Callout queue initialization. */
//	LIST_INIT(&calloutq);
//	callout_add(1, cocb_fps);
//	callout_add(5, cocb_datasrc);
//	callout_add(5, cocb_clearstatus);	/* XXX: enable when PANEL_STATUS? */

	TAILQ_INIT(&panels);
	SLIST_INIT(&selnodes);
	buf_init(&uinp.uinp_buf);
	buf_append(&uinp.uinp_buf, '\0');
	st.st_rf |= RF_INIT;

	/* The first refresh interval will trigger this. */
	st.st_rf &= ~RF_DATASRC;

	/* glutExposeFunc(reshape); */
	glutReshapeFunc(resizeh);
	glutKeyboardFunc(keyh_default);
	glutSpecialFunc(spkeyh_default);
	glutDisplayFunc(drawh);
	glutIdleFunc(idleh_default);
	glutMouseFunc(mouseh_default);
	glutMotionFunc(m_activeh_default);
	glutPassiveMotionFunc(m_passiveh_default);
	glutMainLoop();
	exit(0);
}
