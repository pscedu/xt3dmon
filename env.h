/* $Id$ */

#ifndef _ENV_H_
#define _ENV_H_

#include "xmath.h"

/* Frustum. */
#define NEARCLIP	(1.0)
#define FOVY		(45.0f)
#define ASPECT		(winv.iv_w / (double)winv.iv_h)

/* Relative directions. */
#define DIR_LEFT	0
#define DIR_RIGHT	1
#define DIR_UP		2
#define DIR_DOWN	3
#define DIR_FORWARD	4
#define DIR_BACK	5

#define DIM_X	0
#define DIM_Y	1
#define DIM_Z	2
#define NDIM	3

struct vmode {
	int		 vm_clip;
	struct fvec	 vm_ndim;			/* node dimensions */
};

struct frustum {
	double		 fr_ratio;
	double		 fr_radians;
	double		 fr_wd2;
	double		 fr_ndfl;
	double		 fr_eyesep;
	struct fvec	 fr_stereov;

	float		 fr_left;
	float		 fr_right;
	float		 fr_top;
	float		 fr_bottom;
};

#define FRID_LEFT	0
#define FRID_RIGHT	1

void frustum_init(struct frustum *);
void frustum_calc(int, struct frustum *);

/* Loop through wired repetitions. */
#define WIREP_FOREACH(fvp)							\
	for ((fvp)->fv_x = wi_repstart.fv_x;					\
	    (fvp)->fv_x < wi_repstart.fv_x + wi_repdim.fv_w;			\
	    (fvp)->fv_x += WIV_SWIDTH)						\
		for ((fvp)->fv_y = wi_repstart.fv_y;				\
		    (fvp)->fv_y < wi_repstart.fv_y + wi_repdim.fv_h;		\
		    (fvp)->fv_y += WIV_SHEIGHT)					\
			for ((fvp)->fv_z = wi_repstart.fv_z;			\
			    (fvp)->fv_z < wi_repstart.fv_z + wi_repdim.fv_d;	\
			    (fvp)->fv_z += WIV_SDEPTH)

extern struct vmode	 vmodes[];
extern float		 clip;
extern struct fvec	 wi_repstart, wi_repdim;	/* repeat position & dim */
extern struct ivec	 widim;

extern int		 stereo_mode;
extern int		 window_ids[2];
extern int		 wid;
extern int		 spkey;
extern struct ivec	 mousev;
extern struct ivec	 winv;

#endif /* _ENV_H_ */
