/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cdefs.h"
#include "buf.h"
#include "capture.h"
#include "draw.h"
#include "ds.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "queue.h"
#include "selnode.h"
#include "server.h"
#include "state.h"
#include "uinp.h"
#include "xmath.h"
#include "xssl.h"
#include "yod.h"

#define STARTX		(-30.0)
#define STARTY		( 10.0)
#define STARTZ		( 25.0)

/* Must form a unit vector. */
#define STARTLX		(0.99f)
#define STARTLY		(0.0f)
#define STARTLZ		(-0.12f)

void	usage(void);

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NID_MAX];
struct node		*wimap[WIDIM_WIDTH][WIDIM_HEIGHT][WIDIM_DEPTH];

int			 dsp = DSP_LOCAL;
int			 dsflags = DSFF_ALERT;

struct ivec		 winv = { 800, 600, 0 };

int			 flyby_mode = FBM_OFF;
int			 capture_mode = CM_PPM;
int			 stereo_mode;

struct fvec		 tv = { STARTX, STARTY, STARTZ };
struct fvec		 tlv = { STARTLX, STARTLY, STARTLZ };
struct fvec		 tuv = { 0.0f, 1.0f, 0.0f };

void			(*gl_displayhp)(void);

char			 login_auth[BUFSIZ];

const char		*progname;
int			 verbose;

int			 window_ids[2];
int			 wid = WINID_DEF;		/* current window */

char **sav_argv;

struct xoption opts[] = {
 /*  0 */ { "Texture mapping",		0 },
 /*  1 */ { "Node wireframes",		0 },
 /*  2 */ { "Ground/axes",		0 },
 /*  3 */ { "Camera tweening",		OPF_FBIGN },
 /*  4 */ { "Capture mode",		OPF_HIDE | OPF_FBIGN },
 /*  5 */ { "Display mode",		OPF_HIDE | OPF_FBIGN },
 /*  6 */ { "Govern mode",		OPF_FBIGN },
 /*  7 */ { "Flyby loop mode",		OPF_FBIGN },
 /*  9 */ { "Node labels",		0 },
 /* 10 */ { "Module mode",		0 },
 /* 11 */ { "Wired cluster frames",	0 },
 /* 12 */ { "Pipe mode",		0 },
 /* 13 */ { "Selected node pipe mode",	0 },
 /* 14 */ { "Pause",			OPF_HIDE | OPF_FBIGN },
 /* 15 */ { "Job tour mode",		OPF_FBIGN },
 /* 16 */ { "Skeletons",		0 },
 /* 17 */ { "Node animation",		0 }
};

struct vmode vmodes[] = {
	{ "Physical",		1000,	{ 0.2f, 1.2f, 1.2f } },
	{ "Wired (repeat)",	5,	{ 0.5f, 0.5f, 0.5f } },
	{ "Wired",		1000,	{ 2.0f, 2.0f, 2.0f } }
};

struct dmode dmodes[] = {
 /* 0 */ { "Job" },
 /* 1 */ { "Fail" },
 /* 2 */ { "Temperature" },
 /* 3 */ { "Yod" },
 /* 4 */ { NULL },
 /* 5 */ { NULL },
 /* 6 */ { "Same" },
 /* 7 */ { "Routing Errors" },
};

struct state st = {
	{ STARTX, STARTY, STARTZ },			/* (x,y,z) */
	{ STARTLX, STARTLY, STARTLZ },			/* (lx,ly,lz) */
	{ 0.0f, 1.0f, 0.0f },				/* (ux,uy,uz) */
	OP_WIREFRAME | OP_TWEEN | OP_GROUND | \
	    OP_DISPLAY | OP_NODEANIM,			/* options */
	DM_JOB,						/* which data to show */
	VM_WIREDONE,					/* viewing mode */
	PM_RT,						/* pipe mode */
	HL_ALL,						/* node class to highlight */
	0,
	{ 0, 0, 0 },					/* wired mode offset */
	{ 4, 4, 4 },					/* wired node spacing */
	0						/* rebuild flags */
};

/*
 * Enable options: remove all options already on then flip remaining
 * one specified.  Disable works similarily.
 */
void
opt_enable(int ops)
{
	int i, v, fopts;

	fopts = 0;
	while (ops) {
		i = ffs(ops) - 1;
		v = 1 << i;
		ops &= ~v;
		if ((st.st_opts & v) == 0)
			fopts |= v;
	}
	if (fopts)
		opt_flip(fopts);

}

void
opt_disable(int ops)
{
	int i, v, fopts;

	fopts = 0;
	while (ops) {
		i = ffs(ops) - 1;
		v = 1 << i;
		ops &= ~v;
		if (st.st_opts & v)
			fopts |= v;
	}
	if (fopts)
		opt_flip(fopts);
}

/*
 * Code needed for each option.
 */
void
opt_flip(int fopts)
{
	int i, on;

	st.st_opts ^= fopts;
	while (fopts) {
		i = ffs(fopts) - 1;
		fopts &= ~(1 << i);

		on = st.st_opts & (1 << i);
		status_add("%s %s", opts[i].opt_name,
		    on ? "enabled\n" : "disabled\n");

		switch (1 << i) {
		case OP_TWEEN:
			tv = st.st_v;
			tlv = st.st_lv;
			tuv = st.st_uv;
			break;
		case OP_GOVERN:
			gl_run(gl_setidleh);
			break;
		case OP_CAPTURE:
			if (on)
				capture_begin(capture_mode);
			else
				capture_end();
			break;
		case OP_WIREFRAME:
		case OP_TEX:
		case OP_WIVMFRAME:
		case OP_SELPIPES:
		case OP_NLABELS:
			st.st_rf |= RF_CLUSTER | RF_SELNODE;
			break;
		case OP_SKEL:
		case OP_SHOWMODS:
		case OP_PIPES:
			st.st_rf |= RF_CLUSTER;
			break;
		}
	}
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
dmode_change(void)
{
	struct ivec iv;
	struct node *n;
	int i;

	mode_data_clean = 0;

	switch (st.st_dmode) {
	case DM_JOB:
	case DM_YOD:
		for (i = 0; i < NSC; i++)
			statusclass[i].nc_nmemb = 0;
		break;
	case DM_TEMP:
		for (i = 0; i < NTEMPC; i++)
			tempclass[i].nc_nmemb = 0;
		break;
	case DM_FAIL:
		for (i = 0; i < NFAILC; i++)
			failclass[i].nc_nmemb = 0;
		break;
	case DM_RTUNK:
		for (i = 0; i < NRTC; i++)
			rtclass[i].nc_nmemb = 0;
		break;
	}

	NODE_FOREACH(n, &iv) {
		if (n == NULL)
			continue;

		switch (st.st_dmode) {
		case DM_JOB:
			if (n->n_job)
				n->n_fillp = &n->n_job->j_fill;
			else {
				n->n_fillp = &statusclass[n->n_state].nc_fill;
				statusclass[n->n_state].nc_nmemb++;
			}
			break;
		case DM_YOD:
			if (n->n_yod)
				n->n_fillp = &n->n_yod->y_fill;
			else {
				n->n_fillp = &statusclass[n->n_state].nc_fill;
				statusclass[n->n_state].nc_nmemb++;
			}
			break;
		case DM_TEMP:
			if (n->n_temp == DV_NODATA)
				n->n_fillp = &fill_nodata;
			else {
				i = roundclass(n->n_temp, TEMP_MIN, TEMP_MAX, NTEMPC);
				n->n_fillp = &tempclass[i].nc_fill;
				tempclass[i].nc_nmemb++;
			}
			break;
		case DM_FAIL:
			if (n->n_fails == DV_NODATA)
				n->n_fillp = &fill_nodata;
			else {
				i = roundclass(n->n_fails, FAIL_MIN, FAIL_MAX, NFAILC);
				n->n_fillp = &failclass[i].nc_fill;
				failclass[i].nc_nmemb++;
			}
			break;
		case DM_BORG:
			n->n_fillp = &fill_borg;
			break;
		case DM_MATRIX:
			n->n_fillp = rand() % 2 ? &fill_matrix :
			    &fill_matrix_reloaded;
			break;
		case DM_SAME:
			n->n_fillp = &fill_same;
			break;
		case DM_RTUNK:
			if (n->n_route.rt_err[RP_UNK][rt_type] == DV_NODATA)
				n->n_fillp = &fill_xparent;
			else {
				i = roundclass(n->n_route.rt_err[RP_UNK][rt_type],
				    0, rt_max.rt_err[RP_UNK][rt_type], NRTC);
				n->n_fillp = &rtclass[i].nc_fill;
				rtclass[i].nc_nmemb++;
			}
			break;
		}
	}
}

void
rebuild(int opts)
{
	if (opts & RF_DATASRC) {
		ds_refresh(DS_NODE, dsflags);
		ds_refresh(DS_JOB, dsflags);
		ds_refresh(DS_YOD, dsflags);
		ds_refresh(DS_RT, dsflags);
		ds_refresh(DS_MEM, DSFF_IGN);

		opts |= RF_DMODE | RF_CLUSTER | RF_HLNC;
	}
	if (opts & RF_DMODE) {
		dmode_change();
		opts |= RF_CLUSTER | RF_HLNC;
	}
	if (opts & RF_HLNC) {
		hl_change();
		opts |= RF_CLUSTER;
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
		/*
		 * Resetting any viewing frustrums needs
		 * to be done in the display handler.
		 */
	}
	if (opts & RF_GROUND)
		gl_run(make_ground);
	if (opts & RF_CLUSTER)
		gl_run(make_cluster);
	if (opts & RF_SELNODE)
		gl_run(make_selnodes);
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
	ssl_init();

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
	sh = glutGet(GLUT_SCREEN_HEIGHT) - 30;
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
	else
		panel_toggle(PANEL_HELP);

	gl_run(gl_setup);

	if ((quadric = gluNewQuadric()) == NULL)
		err(1, "gluNewQuadric");

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
