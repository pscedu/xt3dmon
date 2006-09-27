/* $Id$ */

#include "xmath.h"

/* Data mode. */
#define DM_JOB		0
#define DM_FAIL		1
#define DM_TEMP		2
#define DM_YOD		3
#define DM_BORG		4
#define DM_MATRIX	5
#define DM_SAME		6
#define DM_RTUNK	7
#define DM_SEASTAR	8
#define DM_LUSTRE	9
#define NDM		10

/* View mode. */
#define VM_PHYS		0
#define VM_WIRED	1
#define VM_WIONE	2
#define NVM		3

struct state {
	struct fvec	 st_v;			/* camera position */
	struct fvec	 st_lv;			/* camera look direction */
	struct fvec	 st_uv;			/* up direction */
	int		 st_opts;		/* view options */
	int		 st_dmode;		/* data mode */
	int		 st_vmode;		/* view mode */
	int		 st_pipemode;		/* pipe mode */
	int		 st_ssmode;		/* seastar mode */
	int		 st_ssvc;		/* seastar vc */
	int		 st_rtepset;		/* route error port set */
	int		 st_rtetype;		/* route error type */
	int		 st_eggs;		/* Easter eggs */
	struct ivec	 st_wioff;		/* wired mode offsets */
	struct ivec	 st_winsp;		/* wired node spacing */
	int		 st_rf;			/* rebuild flags */
#define st_x st_v.fv_x
#define st_y st_v.fv_y
#define st_z st_v.fv_z

#define st_lx st_lv.fv_x
#define st_ly st_lv.fv_y
#define st_lz st_lv.fv_z

#define st_ux st_uv.fv_x
#define st_uy st_uv.fv_y
#define st_uz st_uv.fv_z
};

/* Pipe modes. */
#define PM_DIR		0
#define PM_RTE		1
#define NPM		2

/* Rebuild flags. */
#define RF_DATASRC	(1<<0)
#define RF_CLUSTER	(1<<1)
#define RF_SELNODE	(1<<2)
#define RF_CAM		(1<<3)
#define RF_GROUND	(1<<4)
#define RF_DMODE	(1<<5)
#define RF_DIM		(1<<6)
#define RF_NODEPHYSV	(1<<7)
#define RF_NODESWIV	(1<<8)
#define RF_VMODE	(1<<9)
#define RF_WIREP	(1<<10)
#define RF_FOCUS	(1<<11)
#define RF_INIT		(RF_DATASRC | RF_CLUSTER | RF_GROUND |		\
			 RF_SELNODE | RF_CAM | RF_DMODE | RF_DIM |	\
			 RF_NODEPHYSV | RF_NODESWIV |  RF_VMODE |	\
			 RF_FOCUS)

#define EGG_BORG 	(1<<0)
#define EGG_MATRIX 	(1<<1)

/* Options. */
#define OP_TEX		(1<<0)
#define OP_FRAMES	(1<<1)
#define OP_GROUND	(1<<2)
#define OP_TWEEN	(1<<3)
#define OP_CAPTURE	(1<<4)
#define OP_DISPLAY	(1<<5)
#define OP_GOVERN	(1<<6)
#define OP_LOOPFLYBY	(1<<7)
#define OP_NLABELS	(1<<8)
#define OP_MODSKELS	(1<<9)
#define OP_PIPES	(1<<10)
#define OP_SELPIPES	(1<<11)
#define OP_STOP		(1<<12)
#define OP_TOURJOB	(1<<13)
#define OP_SKEL		(1<<14)
#define OP_NODEANIM	(1<<15)
#define OP_AUTOFLYBY	(1<<16)
#define OP_REEL		(1<<17)
#define OP_CABSKELS	(1<<18)
#define OP_CAPTION	(1<<19)
#define OP_SUBSET	(1<<20)
#define OP_SELNLABELS	(1<<21)
#define NOPS		22

struct xoption {
	const char	*opt_abbr;
	const char	*opt_name;
	int		 opt_flags;
};

#define OPF_HIDE	(1<<0)		/* hide in panels panel */
#define OPF_FBIGN	(1<<1)		/* ignore in flybys */

int	 rebuild(int);
void	 load_state(struct state *);

extern struct state	 st;
extern struct xoption	 opts[];
