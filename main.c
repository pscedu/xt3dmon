/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "cam.h"
#include "capture.h"
#include "cdefs.h"
#include "deusex.h"
#include "draw.h"
#include "ds.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "job.h"
#include "mach.h"
#include "mark.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "parse.h"
#include "pathnames.h"
#include "phys.h"
#include "queue.h"
#include "reel.h"
#include "selnode.h"
#include "server.h"
#include "state.h"
#include "status.h"
#include "tween.h"
#include "uinp.h"
#include "xmath.h"
#include "xssl.h"

char			 login_auth[BUFSIZ];

const struct fvec	 fv_zero = { { 0.0, 0.0, 0.0 } };
const struct ivec	 iv_zero = { { 0, 0, 0 } };

time_t			 mach_drain;

int			 verbose;
const char		*progname;

char			**sav_argv;

struct xoption options[] = {
 /*  0 */ { "tex",	"Texture mapping",	0 },
 /*  1 */ { "frames",	"Node wireframes",	0 },
 /*  2 */ { "ground",	"Ground/axes",		0 },
 /*  3 */ { "tween",	"Camera tweening",	OPF_FBIGN },
 /*  4 */ { "capture",	"Capture mode",		OPF_HIDE | OPF_FBIGN },
 /*  5 */ { "govern",	"Govern mode",		OPF_FBIGN },
 /*  6 */ { "loop",	"Flyby loop mode",	OPF_FBIGN },
 /*  7 */ { "nlabels",	"Node labels",		0 },
 /*  8 */ { "modskel",	"Blade skeletons",	0 },
 /*  9 */ { "pipes",	"Pipe mode",		0 },
 /* 10 */ { "selpipes",	"Selected node pipes",	0 },
 /* 11 */ { "pause",	"Pause",		OPF_HIDE | OPF_FBIGN },
 /* 12 */ { "skel",	"Skeletons",		0 },
 /* 13 */ { "nodeanim",	"Node animation",	0 },
 /* 14 */ { "autofb",	"Auto flyby mode",	OPF_FBIGN },
 /* 15 */ { "reel",	"Reel mode",		OPF_FBIGN },
 /* 16 */ { "cabskel",	"Cabinet skeletons",	0 },
 /* 17 */ { "caption",	"Captions",		OPF_FBIGN },
 /* 18 */ { "subset",	"Subset mode",		0 },
 /* 19 */ { "selnlbls",	"Selected node labels",	0 },
 /* 20 */ { "capfb",	"Capture Flyby Only",	OPF_FBIGN },
 /* 21 */ { "editfocus","Edit Focus",		0 },
 /* 22 */ { "forcefocus","Force Focus",		0 },
 /* 23 */ { "fancy",	"Fancy Drawing",	OPF_FBIGN },
 /* 24 */ { "cores",	"Active Node Cores",	0 }
};

struct vmode vmodes[] = {
	{ "Physical",		1000,	{ { { 0.2f, 1.2f, 1.2f } }, { { .4f, 1.0f, 1.0f } } } },
	{ "Wired (repeat)",	7,	{ { { 0.5f, 0.5f, 0.5f } }, { { .4f, 1.0f, 1.0f } } } },
	{ "Wired (single)",	1000,	{ { { 2.0f, 2.0f, 2.0f } }, { { .4f, 1.0f, 1.0f } } } },
	{ "Virtual Neighbor",	1000,	{ { { 2.0f, 2.0f, 2.0f } }, { { .4f, 1.0f, 1.0f } } } }
};

struct dmode dmodes[] = {
 /* 0 */ { "job",	"Job" },
 /* 1 */ { "temp",	"Temperature" },
 /* 2 */ { NULL,	NULL },				/* egg */
 /* 3 */ { NULL,	NULL },				/* egg */
 /* 4 */ { "same",	"Same" },
 /* 5 */ { "rte",	"Routing Errors" },
 /* 6 */ { NULL,	"Lustre Status" },
 /* 7 */ { NULL,	NULL }				/* egg */
};

struct state st = {
	{ { STARTX,  STARTY,  STARTZ  } },		/* (x,y,z) */
	{ { STARTLX, STARTLY, STARTLZ } },		/* (lx,ly,lz) */
	0.0, 0,						/* (ur,urev) */
	OP_FRAMES | OP_TWEEN | OP_GROUND |		/* options */
	    OP_NODEANIM | OP_CAPTION | OP_SELNLABELS |
	    OP_CAPFBONLY,
	DM_JOB,						/* which data to show */
	VM_PHYS,					/* viewing mode */
	PM_DIR,						/* pipe mode */
	PDIM_X | PDIM_Y | PDIM_Z,			/* pipe dimensions */
	RPS_NEG,					/* rte port set */
	RT_RECOVER,					/* rte type */
	0,						/* vnmode */
	0,						/* eggs */
	{ { 0, 0, 0 } },				/* wired mode offset */
	{ { 4, 4, 4 } },				/* wired node spacing */
	0						/* rebuild flags */
};

/*
 * Enable options: flip all options specified which are not
 * already enabled.  opt_disable() works similarily.
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

void
opt_set(int opts)
{
	opt_flip(st.st_opts ^ opts);
}

/*
 * Code needed for each option.
 */
void
opt_flip(int fopts)
{
	struct node *n, **np;
	struct physdim *pd;
	const char *s;
	int i, on;

	st.st_opts ^= fopts;
	while (fopts) {
		i = ffs(fopts) - 1;
		fopts &= ~(1 << i);

		on = st.st_opts & (1 << i);
		status_add(SLP_NOTICE,
		    "%s %s", options[i].opt_name,
		    on ? "enabled\n" : "disabled\n");

		switch (1 << i) {
		case OP_TWEEN:
			tween_toggle();
			break;
		case OP_GOVERN:
			gl_setidleh();
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
		case OP_FANCY:
			if (on)
				gl_drawhint = GL_NICEST;
			else
				gl_drawhint = GL_DONT_CARE;
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, gl_drawhint);
			/* FALLTHROUGH */
		case OP_FRAMES:
		case OP_SELPIPES:
		case OP_NLABELS:
		case OP_SELNLABELS:
		case OP_PIPES:
		case OP_CORES:
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
		case OP_NODEANIM:
			if (!on) {
				NODE_FOREACH_PHYS(n, np)
					n->n_v = n->n_vtwe;
				st.st_rf |= RF_CLUSTER;
			}
			break;
		case OP_REEL:
			if (on)
				reel_load();
			break;
		case OP_STOP: {
			static struct fvec sv, slv;
			static float sur;
			static int surev;

			if (on) {
				tween_getdst(&sv, &slv, &sur, &surev);
				tween_setdst(&st.st_v, &st.st_lv,
				    st.st_ur, st.st_urev);
			} else {
				tween_setdst(&sv, &slv, sur, surev);

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
	int surev, opts, rf;
	struct fvec sv, slv;
	float sur;

	sv = st.st_v;
	slv = st.st_lv;
	sur = st.st_ur;
	surev = st.st_urev;

	rf = RF_CAM;
	if (st.st_dmode != nst->st_dmode)
		rf |= RF_DMODE;
	if (st.st_vmode != nst->st_vmode)
		rf |= RF_VMODE;
	if (st.st_pipemode != nst->st_pipemode)
		rf |= RF_CLUSTER | RF_SELNODE;
	if (!ivec_eq(&st.st_wioff, &nst->st_wioff))
		rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | RF_VMODE;
	if (!ivec_eq(&st.st_winsp, &nst->st_winsp))
		rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | RF_VMODE | RF_CAM;
/* XXX: preserve hlnc */
	egg_toggle(st.st_eggs ^ nst->st_eggs);

	opts = st.st_opts;
	st = *nst;
	st.st_opts = opts;
	st.st_rf = rf;

	opt_flip(st.st_opts ^ nst->st_opts);

	if (st.st_opts & OP_TWEEN) {
		tween_setdst(&st.st_v, &st.st_lv, st.st_ur, st.st_urev);

		st.st_v = sv;
		st.st_lv = slv;
		st.st_ur = sur;
		st.st_urev = surev;
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
	struct node **np, *n, *ng;
	int rd, i, port;

	dmode_data_clean = 0;

	switch (st.st_dmode) {
	case DM_JOB:
		for (i = 0; i < NSC; i++)
			statusclass[i].nc_nmemb = 0;
		break;
	case DM_TEMP:
		for (i = 0; i < NTEMPC; i++)
			tempclass[i].nc_nmemb = 0;
		break;
	case DM_RTUNK:
		for (i = 0; i < NRTC; i++)
			rtclass[i].nc_nmemb = 0;
		break;
	case DM_LUSTRE:
		for (i = 0; i < NLUSTC; i++)
			lustreclass[i].nc_nmemb = 0;
		break;
	}

	if (st.st_dmode == DM_RTUNK)
		NODE_FOREACH_PHYS(n, np)
			n->n_fillp = &fill_xparent;

	NODE_FOREACH_PHYS(n, np)
		switch (st.st_dmode) {
		case DM_JOB:
			if (n->n_job)
				n->n_fillp = &n->n_job->j_fill;
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
		case DM_BORG:
			n->n_fillp = &fill_borg;
			break;
		case DM_MATRIX:
			n->n_fillp = rand() % 2 ?
			    &fill_matrix : &fill_matrix_reloaded;
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

void
geom_setall(int mode)
{
	struct node *n, **np;

	NODE_FOREACH_PHYS(n, np)
		n->n_geom = mode;
	// XXX: flyby
	st.st_rf |= RF_DIM | RF_CLUSTER | RF_SELNODE;
}

void
dim_update(void)
{
	struct node *n, **np;

	NODE_FOREACH_PHYS(n, np)
		n->n_dimp = &vmodes[st.st_vmode].vm_ndim[n->n_geom];
}

#include "lnseg.h"

void
vmode_change(void)
{
	int maxhops, nhops, *nneighbors, *ncnt;
	struct fvec spdim, wrapv, fv, lastfv;
	struct node **np, *n, *nodep;
	struct ivec iv, adjv;
	struct selnode *sn;
	struct fvec *nposp;

	if (st.st_vmode == VM_VNEIGHBOR)
		panel_show(PANEL_VNMODE);

	maxhops = 0; /* gcc */
	ncnt = NULL;
	nodep = NULL;
	nneighbors = NULL;
	memset(&spdim, 0, sizeof(spdim));
	switch (st.st_vmode) {
	case VM_VNEIGHBOR:
		/*
		 * As the machine architecture is toruses in three dimensions,
		 * we'll only have to traverse at most half of the cube.
		 */
		maxhops = (widim.iv_w + widim.iv_h + widim.iv_d) / 2 + 1;
		nneighbors = calloc(maxhops, sizeof(*nneighbors));
		if (nneighbors == NULL)
			err(1, "calloc");
		ncnt = calloc(maxhops, sizeof(*ncnt));
		if (ncnt == NULL)
			err(1, "calloc");

		SLIST_FOREACH(sn, &selnodes, sn_next)
			if (sn->sn_nodep->n_job) {
				nodep = sn->sn_nodep;
				break;
			}

		/*
		 * The user does not have a valid one selected, so
		 * traverse each node until we find one we can use.
		 */
		NODE_FOREACH_WI(n, np) {
			if (n->n_job == NULL || n->n_fillp->f_a == 0.0f)
				continue;
			if (nodep == NULL)
				nodep = n;
			else if (nodep->n_job == n->n_job) {
				nhops = node_wineighbor_nhops(nodep, n);
				if (nhops >= maxhops)
					errx(1, "hops greater than max");
				nneighbors[nhops]++;
			}
		}
		break;
	case VM_WIONE:
	case VM_WIRED:
		/* Spaced cluster dimensions. */
		spdim.fv_w = widim.iv_w * st.st_winsp.iv_x;
		spdim.fv_h = widim.iv_h * st.st_winsp.iv_y;
		spdim.fv_d = widim.iv_d * st.st_winsp.iv_z;
		break;
	}

	switch (st.st_vmode) {
	case VM_PHYS:
		NODE_FOREACH_PHYS(n, np) {
			nposp = (st.st_opts & OP_NODEANIM) ?
			    &n->n_vtwe : &n->n_v;
			n->n_flags |= NF_VMVIS;
			node_setphyspos(n, nposp);
		}
		break;
	case VM_VNEIGHBOR:
		NODE_FOREACH_WI(n, np) {
			nposp = (st.st_opts & OP_NODEANIM) ?
			    &n->n_vtwe : &n->n_v;
			if (n == nodep) {
				n->n_flags |= NF_VMVIS;
				vec_set(nposp, 0.0, 0.0, 0.0);
			} else if (nodep && n->n_job == nodep->n_job) {
				n->n_flags |= NF_VMVIS;

				nhops = node_wineighbor_nhops(nodep, n);
				if (nhops >= maxhops)
					errx(1, "hops greater than max");
				fv.fv_r = nhops * 8.0;

				switch (st.st_vnmode) {
				case VNM_A:
					if (nhops == 6) {
#if 0
struct fvec _a, _b;

						fv.fv_t = ncnt[nhops] % (nneighbors[nhops] / 2) *
						    8.0 * M_PI / nneighbors[nhops];
						if (ncnt[nhops] > nneighbors[nhops] / 2)
							fv.fv_t = -fv.fv_t;

fv.fv_p = M_PI * 0.5;
vec_sphere2cart(&fv, &_a);

if (ncnt[nhops] < nneighbors[nhops] / 3)
	fv.fv_p = M_PI * 0.25;
else if (ncnt[nhops] < 2 * nneighbors[nhops] / 3.0)
	fv.fv_p = M_PI * 0.5;
else
	fv.fv_p = M_PI * 0.75;


//						fv.fv_p = M_PI * 0.25 +
//						    ((int)(ncnt[nhops] / 4)) * 4.0 * 0.75 * M_PI / nneighbors[nhops];

vec_sphere2cart(&fv, &_b);
lnseg_add(&_a, &_b);


if (nhops == 5)
  printf("  %.4f %.4f\n", fv.fv_t, fv.fv_p);



#endif

						double h = -1.0 + 2.0 * (ncnt[nhops]) /
						    (double)nneighbors[nhops];
						fv.fv_t = acos(h);
						if (ncnt[nhops] == 0 ||
						    ncnt[nhops] == nneighbors[nhops] - 1)
							fv.fv_p = 0;
						else
							fv.fv_p = fmod(lastfv.fv_p + 3.6 / sqrt(nneighbors[nhops] * (1.0 - SQUARE(h))), 2 * M_PI);
						lastfv.fv_p = fv.fv_p;
					} else {
n->n_flags &= ~NF_VMVIS;
//						fv.fv_t = ncnt[nhops] *
//						    2.0 * M_PI / nneighbors[nhops];
//						fv.fv_p = M_PI * 0.5;
					}

#if 0
	fv.fv_t = ncnt[nhops]++ *
	    2.0 * M_PI / (nneighbors[nhops] / 2);
	fv.fv_p = M_PI / (nneighbors[nhops] / 2);
#endif
					ncnt[nhops]++;
					break;
				case VNM_B:
					fv.fv_t = ncnt[nhops]++ *
					    M_PI / (4.0 * (nhops + 1));
					fv.fv_p = M_PI * 0.5 - ncnt[nhops] * 0.001;
					break;
				case VNM_C:
					fv.fv_t = (ncnt[nhops]++ + 1) / 2 *
					    M_PI / (4.0 * (nhops + 1));
					if (ncnt[nhops] % 2)
						fv.fv_t *= -1.0;
					fv.fv_p = M_PI * 0.5 - ncnt[nhops] * 0.001;
					break;
				case VNM_D:
					fv.fv_t = ncnt[nhops]++ *
					    M_PI / (4.0 * (nhops + 1));
					fv.fv_p = M_PI * 0.5 - nhops * 0.01;
					fv.fv_p = M_PI * 0.5 - ncnt[nhops] * 0.01;
					break;
				}
				vec_sphere2cart(&fv, nposp);
			} else
				n->n_flags &= ~NF_VMVIS;
		}
		break;
	case VM_WIONE:
	case VM_WIRED:
		IVEC_FOREACH(&iv, &widim) {
			adjv.iv_x = negmod(iv.iv_x + st.st_wioff.iv_x, widim.iv_w);
			adjv.iv_y = negmod(iv.iv_y + st.st_wioff.iv_y, widim.iv_h);
			adjv.iv_z = negmod(iv.iv_z + st.st_wioff.iv_z, widim.iv_d);
			n = NODE_WIMAP(adjv.iv_x, adjv.iv_y, adjv.iv_z);
			if (n == NULL)
				continue;
			n->n_flags |= NF_VMVIS;
			nposp = (st.st_vmode == VM_WIONE &&
			    st.st_opts & OP_NODEANIM) ?
			    &n->n_vtwe : &n->n_v;

			wrapv.fv_x = floor((iv.iv_x + st.st_wioff.iv_x) / (double)widim.iv_w) * spdim.fv_w;
			wrapv.fv_y = floor((iv.iv_y + st.st_wioff.iv_y) / (double)widim.iv_h) * spdim.fv_h;
			wrapv.fv_z = floor((iv.iv_z + st.st_wioff.iv_z) / (double)widim.iv_d) * spdim.fv_d;

			nposp->fv_x = n->n_wiv.iv_x * st.st_winsp.iv_x + wrapv.fv_x;
			nposp->fv_y = n->n_wiv.iv_y * st.st_winsp.iv_y + wrapv.fv_y;
			nposp->fv_z = n->n_wiv.iv_z * st.st_winsp.iv_z + wrapv.fv_z;
		}
		break;
	}
	free(nneighbors);
	free(ncnt);
}

/* Snap to nearest space on grid. */
__inline float
snap(float n, float size, float clipval)
{
	float adj;

	adj = fmod(n - clipval, size);
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

__inline int
rf_deps(int opts)
{
	if (opts & RF_DATASRC)
		opts |= RF_VMODE | RF_DMODE | RF_CLUSTER;
	if (opts & RF_DMODE)
		opts |= RF_CLUSTER;
	if (opts & RF_VMODE)
		opts |= RF_CLUSTER | RF_WIREP | RF_CAM | \
		    RF_GROUND | RF_SELNODE | RF_DIM | RF_FOCUS;
	if (opts & RF_DIM)
		opts |= RF_CLUSTER | RF_SELNODE; /* RF_WIREP */
	if (opts & RF_SELNODE)
		opts |= RF_FOCUS;
	return (opts);
}

int
rebuild(int opts)
{
	static int rebuild_wirep;

	if (opts & RF_DATASRC) {
		ds_refresh(DS_DATA, dsfopts);

		/* XXX save mtime and check in panel_refresh_date */
		panel_rebuild(PANEL_DATE);
		caption_setdrain(mach_drain);
	}
	if (opts & RF_DMODE)
		dmode_change();
	if (opts & RF_VMODE)
		vmode_change();
	if (opts & RF_CAM) {
		switch (st.st_vmode) {
		case VM_PHYS:
		case VM_WIONE:
		case VM_VNEIGHBOR:
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
		make_ground();
	if (opts & RF_DIM)
		dim_update();

	if (opts & RF_WIREP)
		rebuild_wirep = 1;
	if (st.st_vmode == VM_WIRED && rebuild_wirep) {
		wirep_update();
		rebuild_wirep = 0;
	}

	if (opts & RF_CLUSTER)
		make_cluster();
	if (opts & RF_SELNODE)
		make_selnodes();
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

void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-dMp] [-c machconf] [-H height] [-W width] [-U uri]\n"
	    "    [-x script]\n", progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int flags, c, sw, sh, j, Mflag;
	const char *cfgfn, *p;
	size_t siz;
	long l;

	progname = argv[0];
	sav_argv = argv;

	arch_init();
	ssl_init();
	ds_setlive();
	srandom(time(NULL));

	cfgfn = _PATH_MACHCONF;

	Mflag = 0;
	flags = GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE;
	glutInit(&argc, argv);
	sw = glutGet(GLUT_SCREEN_WIDTH);
	sh = glutGet(GLUT_SCREEN_HEIGHT) - 30;
	while ((c = getopt(argc, argv, "ac:dH:MpU:vW:x:")) != -1)
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
		case 'H':
			l = strtol(optarg, NULL, 10);
			if (l <= 0 || l > INT_MAX)
				err(1, "invalid screen height: %s", optarg);
			sh = (int)l;
			break;
		case 'M':
			Mflag = 1;
			break;
		case 'p':
			stereo_mode = STM_PASV;
			break;
		case 'U':
			for (j = 0; j < NDS; j++)
				if (datasrcs[j].ds_flags & DSF_LIVE)
					ds_seturi(j, optarg);
			break;
		case 'v':
			verbose++;
			break;
		case 'W':
			l = strtol(optarg, NULL, 10);
			if (l <= 0 || l > INT_MAX)
				err(1, "invalid screen width: %s", optarg);
			sw = (int)l;
			break;
		case 'x':
			snprintf(dx_fn, PATH_MAX, "%s", optarg);
			dx_start();
			break;
		default:
			usage();
			/* NOTREACHED */
		}

	if (stereo_mode == STM_PASV)
		sw /= 2;
	glutInitDisplayMode(flags);
//	glutInitWindowPosition(0, 0);
if (stereo_mode == STM_PASV)
 glutInitWindowPosition(sw, 0);
else
 glutInitWindowPosition(0, 0);
	glutInitWindowSize(sw, sh);
	if ((window_ids[0] = glutCreateWindow("Blacklight Monitor")) == GL_FALSE)
		errx(1, "glutCreateWindow");
	if (stereo_mode == STM_PASV) {
//		glutInitWindowPosition(sw, 0);
glutInitWindowPosition(0, 0);
		if ((window_ids[WINID_RIGHT] =
		    glutCreateWindow("Blacklight Monitor")) == GL_FALSE)
			errx(1, "glutCreateWindow");
	}

	TAILQ_INIT(&panels);
	SLIST_INIT(&selnodes);
	SLIST_INIT(&marks);
	buf_init(&uinp.uinp_buf);
	buf_append(&uinp.uinp_buf, '\0');
	st.st_rf |= RF_INIT;

	parse_colors(_PATH_COLORS);
	parse_machconf(cfgfn);

	siz = NPARTS * NRACKS * NIRUS * NBLADES * NNODES;
	nodes = calloc(siz, sizeof(*nodes));
	if (nodes == NULL)
		err(1, "calloc");
	node_nidmap = calloc(machine.m_nidmax, sizeof(*node_nidmap));
	if (node_nidmap == NULL)
		err(1, "calloc");
	/* XXX overflow */
	node_wimap_len = widim.iv_x * widim.iv_y * widim.iv_z;
	node_wimap = calloc(node_wimap_len, sizeof(*node_wimap));
	if (node_wimap == NULL)
		err(1, "calloc");

	if (server_mode)
		serv_init();

	gl_run(gl_setup);
	gl_setup_core();

	if (!server_mode)
		panel_toggle(PANEL_HELP);

	if ((quadric = gluNewQuadric()) == NULL)
		err(1, "gluNewQuadric");

	if (Mflag)
		glutFullScreen();

	if (st.st_opts & OP_TWEEN)
		tween_toggle();

	p = getenv("XT3DMON_DATA_SRC");
	if (p) {
		strncpy(ds_datasrc, p, sizeof(ds_datasrc) - 1);
		ds_datasrc[sizeof(ds_datasrc) - 1] = '\0';
	}

	glutMainLoop();
	/* NOTREACHED */
	exit(0);
}
