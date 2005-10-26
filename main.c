/* $Id$ */

#include "compat.h"

#include <math.h>
#include <time.h>

#include "cdefs.h"
#include "mon.h"

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
GLint			 cluster_dl[2], ground_dl[2], select_dl[2];
struct timeval		 lastsync;
long			 fps = 50;	/* last fps sample */
long			 fps_cnt = 0;	/* current fps counter */
void			(*drawh)(void);
int			 window_ids[2];
const char		*progname;
int			 verbose;

const char *opdesc[] = {
	/*  0 */ "Texture mode",
	/*  1 */ "Node wireframes",
	/*  2 */ "Ground and axes",
	/*  3 */ "Camera tweening",
	/*  4 */ "Capture mode",
	/*  5 */ "Display mode",
	/*  6 */ "Govern mode",
	/*  7 */ "Flyby loop mode",
	/*  8 */ "Debug mode",
	/*  9 */ "Node labels",
	/* 10 */ "Module mode",
	/* 11 */ "Wired cluster frames",
	/* 12 */ "Pipe mode",
	/* 13 */ "Selected node pipe mode",
	/* 14 */ "Pause",
	/* 15 */ "Job tour mode",
	/* 16 */ "Skeletons"
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
	if (diff & OP_GOVERN)
		glutIdleFunc(st.st_opts & OP_GOVERN ?
		    idleh_govern : idleh_default);
}

void
idleh_govern(void)
{
	static struct timeval gov_tv, tv, diff;

	gettimeofday(&tv, NULL);
	timersub(&tv, &gov_tv, &diff);
	if (diff.tv_sec * 1e6 + diff.tv_usec >= FPS_TO_USEC(GOVERN_FPS)) {
		fps_cnt++;
		gov_tv = tv;
		(*drawh)();
	}
}

void
idleh_default(void)
{
	fps_cnt++;
	(*drawh)();
}

void
make_obj(void (*f)(int))
{
	if (stereo_mode == STM_PASV) {
		glutSetWindow(window_ids[WINID_LEFT]);
		(*f)(WINID_LEFT);
		glutSetWindow(window_ids[WINID_RIGHT]);
		(*f)(WINID_RIGHT);
	} else
		(*f)(WINID_DEF);
}

void
rebuild(int opts)
{
	if (opts & RF_TEX) {
		tex_remove();
		tex_load();
	}
	if (opts & RF_PHYSMAP)
		ds_refresh(DS_PHYS, DSF_CRIT);
	if (opts & RF_SMODE) {
		struct datasrc *ds;
		int dsmode;

		dsmode = -1;
		switch (st.st_mode) {
		case SM_JOBS:
			dsmode = DS_JOBS;
			break;
		case SM_FAIL:
			dsmode = DS_FAIL;
			break;
		case SM_TEMP:
			dsmode = DS_TEMP;
			break;
		}
		ds = ds_get(dsmode);
		ds->ds_flags |= DSF_FORCE;
	}
	if (opts & RF_DATASRC) {
		mode_data_clean = 0;
		switch (st.st_mode) {
		case SM_JOBS:
			ds_refresh(DS_JOBS, 0);
			ds_refresh(DS_QSTAT, 0);
			ds_refresh(DS_BAD, DSF_IGN);
			ds_refresh(DS_CHECK, DSF_IGN);
			break;
		case SM_FAIL:
			ds_refresh(DS_FAIL, 0);
			break;
		case SM_TEMP:
			ds_refresh(DS_TEMP, 0);
			break;
		}
		ds_refresh(DS_MEM, 0);
		hl_refresh();
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
		make_obj(make_ground);
	if (opts & RF_CLUSTER)
		make_obj(make_cluster);
	if (opts & RF_SELNODE)
		make_obj(make_select);
}

char **sav_argv;

void
restart(void)
{
	execvp(sav_argv[0], sav_argv);
	err(1, "execvp");
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-adlpv]\n", progname);
	exit(1);
}

void
setup_glut(void)
{
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
}

int
main(int argc, char *argv[])
{
	int flags, c, sw, sh;
	int server = 0;

	progname = argv[0];
	sav_argv = argv;
	drawh = drawh_default;

	flags = GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE;
	glutInit(&argc, argv);
	while ((c = getopt(argc, argv, "adlpv")) != -1)
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
		case 'v':
			verbose++;
			break;
		default:
			usage();
			/* NOTREACHED */
		}

	/* XXX:  Sanity-check flags. */
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
	setup_glut();
	if (stereo_mode == STM_PASV) {
		glutInitWindowPosition(sw, 0);
		if ((window_ids[WINID_RIGHT] =
		    glutCreateWindow("XT3 Monitor")) == GL_FALSE)
			errx(1, "glutCreateWindow");
		setup_glut();
	}

	glutTimerFunc(1, cocb_fps, 0);
	glutTimerFunc(1, cocb_datasrc, 0);
	glutTimerFunc(1, cocb_clearstatus, 0); /* XXX: enable/disable when PANEL_STATUS? */

	TAILQ_INIT(&panels);
	SLIST_INIT(&selnodes);
	buf_init(&uinp.uinp_buf);
	buf_append(&uinp.uinp_buf, '\0');
	st.st_rf |= RF_INIT;

	if (server)
		serv_init();

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
