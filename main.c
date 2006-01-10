/* $Id$ */

#include "compat.h"

#include <math.h>
#include <time.h>

#include "cdefs.h"
#include "math.h"
#include "mon.h"

#define STARTX		(-30.0f)
#define STARTY		(10.0f)
#define STARTZ		(25.0f)

#define STARTLX		(0.99f)		/* Must form a unit vector. */
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

void	usage(void);

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NID_MAX];
struct node		*wimap[WIDIM_WIDTH][WIDIM_HEIGHT][WIDIM_DEPTH];

int			 dsp = DSP_LOCAL;

int			 win_width = 800;
int			 win_height = 600;

int			 flyby_mode = FBM_OFF;
int			 capture_mode = CM_PPM;
int			 stereo_mode;

struct fvec		 tv = { STARTX, STARTY, STARTZ };
struct fvec		 tlv = { STARTLX, STARTLY, STARTLZ };
struct fvec		 tuv = { 0.0f, 1.0f, 0.0f };

GLint			 cluster_dl[2], ground_dl[2], select_dl[2];

void			(*gl_displayhp)(void);

const char		*progname;
int			 verbose;

int			 window_ids[2];
int			 wid = WINID_DEF;		/* current window */

char **sav_argv;

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
	/* 16 */ "Skeletons",
	/* 17 */ "Node animation"
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
	OP_WIREFRAME | OP_TWEEN | OP_GROUND | OP_DISPLAY |
	    OP_NODEANIM,					/* options */
	SM_JOB,							/* which data to show */
	VM_PHYSICAL,						/* viewing mode */
	{ 4, 4, 4 },						/* wired node spacing */
	0							/* rebuild flags (unused) */
};

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
		gl_run(gl_setidleh);
}

int
roundclass(int t, int min, int max, int nclasses)
{
	if (t < min)
		t = min;
	else if (t > max)
		t = max;
	return ((t - min) * nclasses / (max - min));
}

void
smode_change(void)
{
	struct ivec iv;
	struct node *n;

	IVEC_FOREACH(&iv, &widim) {
		n = wimap[iv.iv_x][iv.iv_y][iv.iv_z];
		if (n == NULL)
			continue;

		switch (st.st_mode) {
		case SM_JOB:
			if (n->n_job)
				n->n_fillp = &n->n_job->j_fill;
			else
				n->n_fillp = &statusclass[n->n_state].nc_fill;
			break;
		case SM_YOD:
			if (n->n_yod)
				n->n_fillp = &n->n_yod->y_fill;
			else
				n->n_fillp = &statusclass[n->n_state].nc_fill;
			break;
		case SM_TEMP:
			if (n->n_temp != DV_NODATA)
				n->n_fillp = &tempclass[roundclass(n->n_temp,
				    TEMP_MIN, TEMP_MAX, TEMP_NTEMPS)].nc_fill;
			else
				n->n_fillp = &fill_nodata;
			break;
		case SM_FAIL:
			if (n->n_fails != DV_NODATA)
				n->n_fillp = &failclass[roundclass(n->n_fails,
			    FAIL_MIN, FAIL_MAX, FAIL_NFAILS)].nc_fill;
			else
				n->n_fillp = &fill_nodata;
			break;
		}
	}
}

void
rebuild(int opts)
{
	if (opts & RF_SMODE) {
		struct datasrc *ds;
		int dsmode;

		dsmode = -1;
		switch (st.st_mode) {
		case SM_JOB:
			dsmode = DS_JOB;
			break;
		case SM_YOD:
			dsmode = DS_YOD;
			break;
		}
		/* Force update when changing modes. */
		if (dsmode != -1) {
			ds = ds_get(dsmode);
			ds->ds_flags |= DSF_FORCE;
		}
	}
	if (opts & RF_DATASRC) {
		mode_data_clean = 0;
		ds_refresh(DS_NODE, 0);
		ds_refresh(DS_JOB, 0);
		ds_refresh(DS_YOD, 0);
		ds_refresh(DS_MEM, DSF_IGN);
		hl_refresh();

		opts |= RF_SMODE;
	}
	if (opts & RF_SMODE)
		smode_change();
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
		gl_run(make_ground);
	if (opts & RF_CLUSTER)
		gl_run(make_cluster);
	if (opts & RF_SELNODE)
		gl_run(make_select);
}

void
restart(void)
{
	execvp(sav_argv[0], sav_argv);
	err(1, "execvp");
}

int
main(int argc, char *argv[])
{
	int flags, c, sw, sh;
	int server = 0;

	progname = argv[0];
	sav_argv = argv;

	arch_init();

	gl_displayhp = gl_displayh_default;

	flags = GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE;
	glutInit(&argc, argv);
	while ((c = getopt(argc, argv, "adpv")) != -1)
		switch (c) {
		case 'a':
			flags |= GLUT_STEREO;
			gl_displayhp = gl_displayh_stereo;
			stereo_mode = STM_ACT;
			break;
		case 'd':
			server = 1;
			break;
		case 'p':
			gl_displayhp = gl_displayh_stereo;
			stereo_mode = STM_PASV;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
			/* NOTREACHED */
		}

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
//		glutDisplayFunc(null)
		glutInitWindowPosition(sw, 0);
		if ((window_ids[WINID_RIGHT] =
		    glutCreateWindow("XT3 Monitor")) == GL_FALSE)
			errx(1, "glutCreateWindow");
	}

	TAILQ_INIT(&panels);
	SLIST_INIT(&selnodes);
	buf_init(&uinp.uinp_buf);
	buf_append(&uinp.uinp_buf, '\0');
	st.st_rf |= RF_INIT;

	if (server)
		serv_init();

	gl_run(gl_setup);
	glutMainLoop();
	/* NOTREACHED */
	exit(0);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-adpv]\n", progname);
	exit(1);
}
