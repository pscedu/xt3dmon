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

#include "xmath.h"

/* Data mode. */
#define DM_JOB		0
#define DM_TEMP		1
#define DM_BORG		2	/* egg */
#define DM_MATRIX	3	/* egg */
#define DM_SAME		4
#define DM_RTUNK	5
#define DM_LUSTRE	6
#define DM_HACKERS	7	/* egg */
#define NDM		8

/* View mode. */
#define VM_PHYS		0
#define VM_WIRED	1
#define VM_WIONE	2
#define VM_VNEIGHBOR	3
#define NVM		4

struct state {
	struct fvec	 st_v;			/* camera position */
	struct fvec	 st_lv;			/* camera look direction */
	float		 st_ur;			/* #radians "up" is rotated */
	int		 st_urev;		/* whether "up" is reversed */
	int		 st_opts;		/* view options */
	int		 st_dmode;		/* data mode */
	int		 st_vmode;		/* view mode */
	int		 st_pipemode;		/* pipe mode */
	int		 st_pipedim;		/* which pipe dimensions */
	int		 st_rtepset;		/* route error port set */
	int		 st_rtetype;		/* route error type */
	int		 st_vnmode;		/* VNEIGHBOR mode */
	int		 st_eggs;		/* Easter eggs */
	struct ivec	 st_wioff;		/* wired mode offsets */
	struct ivec	 st_winsp;		/* wired node spacing */
	int		 st_rf;			/* rebuild flags */
#define st_x st_v.fv_x
#define st_y st_v.fv_y
#define st_z st_v.fv_z
};

#define VNM_A		0
#define VNM_B		1
#define VNM_C		2
#define VNM_D		3
#define NVNM		4

/* Pipe modes */
#define PM_DIR		0
#define PM_RTE		1
#define NPM		2

/* Pipe dimensions */
#define PDIM_X		(1 <<0)
#define PDIM_Y		(1 <<1)
#define PDIM_Z		(1 <<2)

/* Rebuild flags */
#define RF_DATASRC	(1 << 0)
#define RF_CLUSTER	(1 << 1)
#define RF_SELNODE	(1 << 2)
#define RF_CAM		(1 << 3)
#define RF_GROUND	(1 << 4)
#define RF_DMODE	(1 << 5)
#define RF_DIM		(1 << 6)
#define RF_VMODE	(1 << 8)
#define RF_WIREP	(1 << 9)
#define RF_FOCUS	(1 << 9)
#define RF_INIT		(RF_DATASRC | RF_CLUSTER | RF_GROUND |		\
			 RF_SELNODE | RF_CAM | RF_DMODE | RF_DIM |	\
			 RF_VMODE | RF_FOCUS)
#define RF_STEREO	(RF_CLUSTER | RF_GROUND | RF_SELNODE)

#define EGG_BORG	(1 << 0)
#define EGG_MATRIX	(1 << 1)
#define EGG_HACKERS	(1 << 2)

/* Options */
#define OP_TEX		(1 << 0)
#define OP_FRAMES	(1 << 1)
#define OP_GROUND	(1 << 2)
#define OP_TWEEN	(1 << 3)
#define OP_CAPTURE	(1 << 4)
#define OP_GOVERN	(1 << 5)
#define OP_LOOPFLYBY	(1 << 6)
#define OP_NLABELS	(1 << 7)
#define OP_MODSKELS	(1 << 8)
#define OP_PIPES	(1 << 9)
#define OP_SELPIPES	(1 << 10)
#define OP_STOP		(1 << 11)
#define OP_SKEL		(1 << 12)
#define OP_NODEANIM	(1 << 13)
#define OP_AUTOFLYBY	(1 << 14)
#define OP_REEL		(1 << 15)
#define OP_CABSKELS	(1 << 16)
#define OP_CAPTION	(1 << 17)
#define OP_SUBSET	(1 << 18)
#define OP_SELNLABELS	(1 << 19)
#define OP_CAPFBONLY	(1 << 20)
#define OP_EDITFOCUS	(1 << 21)
#define OP_FORCEFOCUS	(1 << 22)
#define OP_FANCY	(1 << 23)
#define OP_CORES	(1 << 24)
#define NOPS		25

struct xoption {
	const char	*opt_abbr;
	const char	*opt_name;
	int		 opt_flags;
};

#define OPF_HIDE	(1<<0)		/* hide in panels panel */
#define OPF_FBIGN	(1<<1)		/* ignore in flybys */

int	 rebuild(int);
int	 rf_deps(int);
void	 load_state(struct state *);

extern struct state	 st;
extern struct xoption	 options[];
