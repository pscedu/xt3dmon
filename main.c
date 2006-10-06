/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "buf.h"
#include "capture.h"
#include "deusex.h"
#include "draw.h"
#include "ds.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "job.h"
#include "mark.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "pathnames.h"
#include "phys.h"
#include "queue.h"
#include "reel.h"
#include "selnode.h"
#include "server.h"
#include "state.h"
#include "uinp.h"
#include "xmath.h"
#include "xssl.h"
#include "yod.h"

#define STARTX		(-17.80)
#define STARTY		( 30.76)
#define STARTZ		( 51.92)

#define STARTLX		( 0.71)
#define STARTLY		(-0.34)
#define STARTLZ		(-0.62)

#define STARTUX		( 0.25)
#define STARTUY		( 0.94)
#define STARTUZ		(-0.22)

void	usage(void);

struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
struct node		*invmap[NID_MAX];
struct node		*wimap[WIDIM_WIDTH][WIDIM_HEIGHT][WIDIM_DEPTH];

struct fvec		 tv  = { { STARTX,  STARTY,  STARTZ } };
struct fvec		 tlv = { { STARTLX, STARTLY, STARTLZ } };
struct fvec		 tuv = { { STARTUX, STARTUY, STARTUZ } };

char			 login_auth[BUFSIZ];

const struct fvec	 fv_zero = { { 0.0, 0.0, 0.0 } };
const struct ivec	 iv_zero = { { 0, 0, 0 } };

time_t			 mach_drain;

int			 verbose;
const char		*progname;

char			**sav_argv;

struct xoption opts[] = {
 /*  0 */ { "tex",	"Texture mapping",		0 },
 /*  1 */ { "frames",	"Node wireframes",		0 },
 /*  2 */ { "ground",	"Ground/axes",			0 },
 /*  3 */ { "tween",	"Camera tweening",		OPF_FBIGN },
 /*  4 */ { "capture",	"Capture mode",			OPF_HIDE | OPF_FBIGN },
 /*  5 */ { "govern",	"Govern mode",			OPF_FBIGN },
 /*  6 */ { "loop",	"Flyby loop mode",		OPF_FBIGN },
 /*  7 */ { "nlabels",	"Node labels",			0 },
 /*  8 */ { "modskel",	"Blade skeletons",		0 },
 /*  9 */ { "pipes",	"Pipe mode",			0 },
 /* 10 */ { "selpipes",	"Selected node pipes",		0 },
 /* 11 */ { "pause",	"Pause",			OPF_HIDE | OPF_FBIGN },
 /* 12 */ { "tour",	"Job tour mode",		OPF_FBIGN },
 /* 13 */ { "skel",	"Skeletons",			0 },
 /* 14 */ { "nodeanim",	"Node animation",		0 },
 /* 15 */ { "autofb",	"Auto flyby mode",		OPF_FBIGN },
 /* 16 */ { "reel",	"Reel mode",			OPF_FBIGN },
 /* 17 */ { "cabskel",	"Cabinet skeletons",		0 },
 /* 18 */ { "caption",	"Captions",			OPF_FBIGN },
 /* 19 */ { "subset",	"Subset mode",			0 },
 /* 20 */ { "selnlbls",	"Selected node labels",		0 }
};

struct vmode vmodes[] = {
	{ "Physical",		1000,	{ { { 0.2f, 1.2f, 1.2f } }, { { .4f, 1.0f, 1.0f } } } },
	{ "Wired (repeat)",	7,	{ { { 0.5f, 0.5f, 0.5f } }, { { .4f, 1.0f, 1.0f } } } },
	{ "Wired (single)",	1000,	{ { { 2.0f, 2.0f, 2.0f } }, { { .4f, 1.0f, 1.0f } } } }
};

struct dmode dmodes[] = {
 /* 0 */ { "Job" },
 /* 1 */ { NULL },					/* Failure */
 /* 2 */ { "Temperature" },
 /* 3 */ { "Yod" },
 /* 4 */ { NULL },					/* Egg */
 /* 5 */ { NULL },					/* Egg */
 /* 6 */ { "Same" },
 /* 7 */ { "Routing Errors" },
 /* 8 */ { "Seastar" },
 /* 9 */ { "Lustre Status" }
};

struct state st = {
	{ { STARTX,  STARTY,  STARTZ  } },		/* (x,y,z) */
	{ { STARTLX, STARTLY, STARTLZ } },		/* (lx,ly,lz) */
	{ { STARTUX, STARTUY, STARTUZ } },		/* (ux,uy,uz) */
	OP_FRAMES | OP_TWEEN | OP_GROUND | \
	    OP_NODEANIM | OP_CAPTION | OP_SELNLABELS,	/* options */
	DM_JOB,						/* which data to show */
	VM_PHYS,					/* viewing mode */
	PM_DIR,						/* pipe mode */
	SSCNT_NBLK,					/* seastar mode */
	0,						/* seastar vc */
	RPS_NEG,					/* rte port set */
	RT_RECOVER,					/* rte type */
	0,						/* eggs */
	{ { 0, 0, 0 } },				/* wired mode offset */
	{ { 4, 4, 4 } },				/* wired node spacing */
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
	struct physdim *pd;
	const char *s;
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
			gl_setidleh();
			break;
		case OP_CAPTURE:
			if (on)
				capture_begin(capture_mode);
			else
				capture_end();
			break;
		case OP_TEX:
			if (on) {
				nc_runall(fill_tex);
				fill_selnode.f_flags |= FF_TEX;
			} else {
				nc_runall(fill_untex);
				fill_selnode.f_flags &= ~FF_TEX;
			}
			st.st_rf |= RF_CLUSTER | RF_SELNODE;
			break;
		case OP_FRAMES:
		case OP_SELPIPES:
		case OP_NLABELS:
		case OP_SELNLABELS:
		case OP_PIPES:
			st.st_rf |= RF_CLUSTER | RF_SELNODE;
			break;
		case OP_MODSKELS:
		case OP_CABSKELS:
			s = (1 << i) == OP_CABSKELS ? "cabinet" : "blade";
			for (pd = physdim_top; pd; pd = pd->pd_contains)
				if (strcmp(pd->pd_name, s) == 0) {
					if (on)
						pd->pd_flags |= PDF_SKEL;
					else
						pd->pd_flags &= ~PDF_SKEL;
					break;
				}
			st.st_rf |= RF_CLUSTER;
			break;
		case OP_SKEL:
		case OP_SUBSET:
			st.st_rf |= RF_CLUSTER;
			break;
		case OP_AUTOFLYBY:
			if (on) {
				flyby_nautoto = 0;
				flyby_rstautoto();
			}
			break;
		case OP_REEL:
			if (on)
				reel_load();
			break;
		case OP_STOP: {
			static struct fvec sv, slv, suv;

			if (on) {
				sv = tv;
				slv = tlv;
				suv = tuv;

				tv = st.st_v;
				tlv = st.st_lv;
				tuv = st.st_uv;
			} else {
				tv = sv;
				tlv = slv;
				tuv = suv;

				/*
				 * Rebuild the cluster because
				 * RF_CLUSTER would get cleared
				 * during node tweening with OP_STOP.
				 */
				st.st_rf |= RF_CAM | RF_CLUSTER | RF_SELNODE;
			}
			break;
		    }
		}
	}
}

void
load_state(struct state *nst)
{
	struct fvec sv, slv, suv;
	int opts, rf;

	sv = st.st_v;
	slv = st.st_lv;
	suv = st.st_uv;

	rf = RF_CAM;
	if (st.st_dmode != nst->st_dmode)
		rf |= RF_DMODE;
	if (st.st_vmode != nst->st_vmode)
		rf |= RF_VMODE;
	if (st.st_pipemode != nst->st_pipemode)
		rf |= RF_CLUSTER | RF_SELNODE;
	if (!ivec_eq(&st.st_wioff, &nst->st_wioff))
		rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | RF_NODESWIV;
	if (!ivec_eq(&st.st_winsp, &nst->st_winsp))
		rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | RF_NODESWIV | RF_CAM;
/* XXX: preserve hlnc */
	egg_toggle(st.st_eggs ^ nst->st_eggs);

	opts = st.st_opts;
	st = *nst;
	st.st_opts = opts;
	st.st_rf = rf;

	opt_flip(st.st_opts ^ nst->st_opts);

	if (st.st_opts & OP_TWEEN) {
		tv = st.st_v;
		tlv = st.st_lv;
		tuv = st.st_uv;

		st.st_v = sv;
		st.st_lv = slv;
		st.st_uv = suv;
	}
}

#if 0
int
roundclass_lg(uint64_t t, uint64_t min, uint64_t max, int nclasses)
{
	int i, bitspercl, nbits, mask;

	if (max - min == 0)
		return (0);

	if (t < min)
		t = min;
	else if (t > max)
		t = max;

	nbits = sizeof(t) * 8;
	bitspercl = nbits / nclasses;
	mask = 0;
	for (i = 0; i < bitspercl; i++)
		mask |= (1 << i);

	for (i = nclasses - 1; i >= 0; i--)
		if (t & (mask << (i * bitspercl)))
			return (i);
	return (0);
}
#endif

int
roundclass(double t, double min, double max, int nclasses)
{
	if (max - min == 0)
		return (0);

	if (t < min)
		t = min;
	else if (t > max)
		t = max;

	return ((t - min) / ((max - min) / nclasses + 1e-10));
}

void
dmode_change(void)
{
	struct node *n, *ng;
	int rd, i, port;
	struct ivec iv;

	dmode_data_clean = 0;

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
	case DM_SEASTAR:
		for (i = 0; i < NSSC; i++)
			ssclass[i].nc_nmemb = 0;
		break;
	case DM_LUSTRE:
		for (i = 0; i < NLUSTC; i++)
			lustreclass[i].nc_nmemb = 0;
		break;
	}

	if (st.st_dmode == DM_RTUNK)
		NODE_FOREACH(n, &iv)
			if (n)
				n->n_fillp = &fill_xparent;

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
			for (i = 0; i < NDIM; i++) {
				port = DIM_TO_PORT(i, st.st_rtepset);
				if (n->n_route.rt_err[port][st.st_rtetype]) {
					n->n_fillp = &fill_rtesnd;

					rd = rteport_to_rdir(port);
					ng = node_neighbor(VM_WIRED, n, rd, NULL);
					if (ng->n_fillp == &fill_xparent)
						ng->n_fillp = &fill_rtercv;
				}
			}
			if (n->n_route.rt_err[RP_UNK][st.st_rtetype]) {
				i = roundclass(n->n_route.rt_err[RP_UNK][st.st_rtetype],
				    0, rt_max.rt_err[RP_UNK][st.st_rtetype], NRTC);
				n->n_fillp = &rtclass[i].nc_fill;
				rtclass[i].nc_nmemb++;
			}
			break;
		case DM_SEASTAR:
			i = roundclass(n->n_sstar.ss_cnt[st.st_ssmode][st.st_ssvc],
			    0, ss_max.ss_cnt[st.st_ssmode][st.st_ssvc], NSSC);
			n->n_fillp = &ssclass[i].nc_fill;
			ssclass[i].nc_nmemb++;
			break;
		case DM_LUSTRE:
			n->n_fillp = &lustreclass[n->n_lustat].nc_fill;
			lustreclass[n->n_lustat].nc_nmemb++;
			break;
//		default:
//			n->n_fillp = &fill_xparent;
//			dbg_crash();
//			break;
		}
	}
}

__inline void
rebuild_nodephysv(void)
{
	struct node *n;
	struct ivec iv;

	NODE_FOREACH(n, &iv)
		if (n)
			node_setphyspos(n, &n->n_physv);
}

__inline void
rebuild_nodeswiv(void)
{
	struct fvec spdim, wrapv;
	struct ivec iv, adjv;
	struct node *n;

	/* Spaced cluster dimensions. */
	spdim.fv_w = widim.iv_w * st.st_winsp.iv_x;
	spdim.fv_h = widim.iv_h * st.st_winsp.iv_y;
	spdim.fv_d = widim.iv_d * st.st_winsp.iv_z;

	IVEC_FOREACH(&iv, &widim) {
		adjv.iv_x = negmod(iv.iv_x + st.st_wioff.iv_x, widim.iv_w);
		adjv.iv_y = negmod(iv.iv_y + st.st_wioff.iv_y, widim.iv_h);
		adjv.iv_z = negmod(iv.iv_z + st.st_wioff.iv_z, widim.iv_d);
		n = wimap[adjv.iv_x][adjv.iv_y][adjv.iv_z];
		if (n == NULL)
			continue;

		wrapv.fv_x = floor((iv.iv_x + st.st_wioff.iv_x) / (double)widim.iv_w) * spdim.fv_w;
		wrapv.fv_y = floor((iv.iv_y + st.st_wioff.iv_y) / (double)widim.iv_h) * spdim.fv_h;
		wrapv.fv_z = floor((iv.iv_z + st.st_wioff.iv_z) / (double)widim.iv_d) * spdim.fv_d;

		n->n_swiv.fv_x = n->n_wiv.iv_x * st.st_winsp.iv_x + wrapv.fv_x;
		n->n_swiv.fv_y = n->n_wiv.iv_y * st.st_winsp.iv_y + wrapv.fv_y;
		n->n_swiv.fv_z = n->n_wiv.iv_z * st.st_winsp.iv_z + wrapv.fv_z;
	}
}

void
geom_setall(int mode)
{
	struct node *n;
	struct ivec iv;

	NODE_FOREACH(n, &iv)
		if (n)
			n->n_geom = mode;
	// XXX: flyby
	st.st_rf |= RF_DIM | RF_CLUSTER | RF_SELNODE;
}

void
dim_update(void)
{
	struct node *n;
	struct ivec iv;

	NODE_FOREACH(n, &iv)
		if (n)
			n->n_dimp = &vmodes[st.st_vmode].vm_ndim[n->n_geom];
}

void
vmode_change(void)
{
	struct node *n;
	struct ivec iv;

	NODE_FOREACH(n, &iv)
		if (n)
			n->n_v = (st.st_vmode == VM_PHYS) ?
			    &n->n_physv : &n->n_swiv;
}

/* Snap to nearest space on grid. */
__inline float
snap(float n, float size, float clip)
{
	float adj;

	adj = fmod(n - clip, size);
//	while (adj < 0)
	if (adj < 0)
		adj += size;
	return (adj);
}

__inline void
wirep_update(void)
{
	struct fvec dim;
	float x, y, z;

	x = st.st_x - clip;
	y = st.st_y - clip;
	z = st.st_z - clip;

	/*
	 * Snapping too close is okay because they nodes
	 * will seem to just "appear" regardless.
	 */
	x -= snap(x, WIV_SWIDTH, 0.0);
	y -= snap(y, WIV_SHEIGHT, 0.0);
	z -= snap(z, WIV_SDEPTH, 0.0);

	wi_repstart.fv_x = x;
	wi_repstart.fv_y = y;
	wi_repstart.fv_z = z;

	dim.fv_w = WIV_SWIDTH;
	dim.fv_h = WIV_SHEIGHT;
	dim.fv_d = WIV_SDEPTH;

	wi_repdim.fv_w = dim.fv_w * (int)howmany(st.st_x + clip - x, dim.fv_w);
	wi_repdim.fv_h = dim.fv_h * (int)howmany(st.st_y + clip - y, dim.fv_h);
	wi_repdim.fv_d = dim.fv_d * (int)howmany(st.st_z + clip - z, dim.fv_d);
}

int
rebuild(int opts)
{
	static int rebuild_wirep;

	if (opts & RF_DATASRC) {
		ds_refresh(DS_NODE, dsflags);
		ds_refresh(DS_JOB, dsflags);
		ds_refresh(DS_YOD, dsflags);
		ds_refresh(DS_RT, DSFF_IGN);
		ds_refresh(DS_SS, DSFF_IGN);
		ds_refresh(DS_MEM, DSFF_IGN);

		opts |= RF_DMODE | RF_CLUSTER;

		/* XXX save mtime and check in panel_refresh_date */
		panel_rebuild(PANEL_DATE);
		caption_setdrain(mach_drain);
	}
	if (opts & RF_DMODE) {
		dmode_change();
		opts |= RF_CLUSTER;
	}
	if (opts & RF_VMODE) {
		vmode_change();

		opts |= RF_CLUSTER | RF_NODESWIV | RF_WIREP | RF_CAM | \
		    RF_GROUND | RF_SELNODE | RF_DIM | RF_FOCUS;
	}
	if (opts & RF_CAM) {
		switch (st.st_vmode) {
		case VM_PHYS:
		case VM_WIONE:
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
	if (opts & RF_DIM) {
		dim_update();

		opts |= RF_CLUSTER | RF_SELNODE; /* RF_WIREP */
	}
	if (opts & RF_NODEPHYSV)
		rebuild_nodephysv();
	if (opts & RF_NODESWIV) {
		rebuild_nodeswiv();

		opts |= RF_WIREP | RF_FOCUS;
	}

	if (opts & RF_WIREP)
		rebuild_wirep = 1;
	if (st.st_vmode == VM_WIRED && rebuild_wirep) {
		wirep_update();
		rebuild_wirep = 0;
	}

	if (opts & RF_CLUSTER)
		gl_run(make_cluster);
	if (opts & RF_SELNODE) {
		gl_run(make_selnodes);

		opts |= RF_FOCUS;
	}
	if (opts & RF_FOCUS) {
		if (SLIST_EMPTY(&selnodes))
			focus_cluster(&focus);
		else
			focus_selnodes(&focus);
	}
	return (opts);
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
	int Nflag, flags, c, sw, sh;
	const char *cfgfn;

	progname = argv[0];
	sav_argv = argv;

	arch_init();
	ssl_init();
	srandom(time(NULL));

	cfgfn = _PATH_PHYSCONF;

	Nflag = 0;
	flags = GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE;
	glutInit(&argc, argv);
	while ((c = getopt(argc, argv, "ac:dpv")) != -1)
		switch (c) {
		case 'a':
errx(1, "broken");
			flags |= GLUT_STEREO;
			stereo_mode = STM_ACT;
			break;
		case 'c':
			cfgfn = optarg;
			break;
		case 'd':
			server_mode = 1;
			break;
		case 'p':
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
		glutInitWindowPosition(sw, 0);
		if ((window_ids[WINID_RIGHT] =
		    glutCreateWindow("XT3 Monitor")) == GL_FALSE)
			errx(1, "glutCreateWindow");
	}

	TAILQ_INIT(&panels);
	SLIST_INIT(&selnodes);
	SLIST_INIT(&marks);
	buf_init(&uinp.uinp_buf);
	buf_append(&uinp.uinp_buf, '\0');
	st.st_rf |= RF_INIT;

	parse_colors(_PATH_COLORS);
	parse_physconf(cfgfn);

	if (server_mode)
		serv_init();

	gl_run(gl_setup);
	gl_setup_core();

	if (!server_mode)
		panel_toggle(PANEL_HELP);

	if ((quadric = gluNewQuadric()) == NULL)
		err(1, "gluNewQuadric");

	glutMainLoop();
	/* NOTREACHED */
	exit(0);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-dp] [-c physconf]\n", progname);
	exit(1);
}
