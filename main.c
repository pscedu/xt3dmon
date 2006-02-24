/* $Id$ */

#include "compat.h"

#include <math.h>
#include <time.h>

#include "cdefs.h"
#include "mon.h"
#include "xmath.h"

#define STARTX		(-30.0)
#define STARTY		( 10.0)
#define STARTZ		( 25.0)

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

struct xoption opts[] = {
	/*  0 */ { "Texture mapping",		0 },
	/*  1 */ { "Node wireframes",		0 },
	/*  2 */ { "Ground/axes",		0 },
	/*  3 */ { "Camera tweening",		0 },
	/*  4 */ { "Capture mode",		OPF_HIDE },
	/*  5 */ { "Display mode",		OPF_HIDE },
	/*  6 */ { "Govern mode",		0 },
	/*  7 */ { "Flyby loop mode",		0 },
	/*  9 */ { "Node labels",		0 },
	/* 10 */ { "Module mode",		0 },
	/* 11 */ { "Wired cluster frames",	0 },
	/* 12 */ { "Pipe mode",			0 },
	/* 13 */ { "Selected node pipe mode",	0 },
	/* 14 */ { "Pause",			OPF_HIDE },
	/* 15 */ { "Job tour mode",		0 },
	/* 16 */ { "Skeletons",			0 },
	/* 17 */ { "Node animation",		0 }
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

/* Special case of base 2 to base 10. */
int
baseconv(int n)
{
	unsigned int i;

	for (i = 0; i < sizeof(n) * 8; i++)
		if (n & (1 << i))
			return (i + 1);
	return (0);
}

void
smode_change(void)
{
	struct ivec iv;
	struct node *n;

	mode_data_clean = 0;
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
		case SM_BORG:
			n->n_fillp = &fill_borg;
			break;
		case SM_MATRIX:
			n->n_fillp = rand() % 2 ? &fill_matrix :
			    &fill_matrix_reloaded;
			break;
		}
	}
}

void
rebuild(int opts)
{
#if 0
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
//		if (dsmode != -1) {
//			ds = ds_get(dsmode);
//			ds->ds_flags |= DSF_FORCE;
//		}
	}
#endif
	if (opts & RF_DATASRC) {
		ds_refresh(DS_NODE, 0);
		ds_refresh(DS_JOB, 0);
		ds_refresh(DS_YOD, 0);
		ds_refresh(DS_MEM, DSF_IGN);
		hl_refresh();

		opts |= RF_SMODE | RF_CLUSTER;
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
