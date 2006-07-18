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
#define DIR_FORW	4
#define DIR_BACK	5

/* Dimensions. */
#define DIM_X		0
#define DIM_Y		1
#define DIM_Z		2
#define NDIM		3

/* Relative directions. */
#define RD_NEGX		0
#define RD_POSX		1
#define RD_NEGY		2
#define RD_POSY		3
#define RD_NEGZ		4
#define RD_POSZ		5

/* Geometries. */
#define GEOM_CUBE	0
#define GEOM_SPHERE	1
#define NGEOM		2

/* Stereo display mode types. */
#define STM_NONE	0
#define STM_ACT		1
#define STM_PASV	2

/* GL window identifiers for passive stereo. */
#define WINID_LEFT	0
#define WINID_RIGHT	1

#define WINID_DEF	0

/* Key handlers. */
#define KEYH_DEF	0
#define KEYH_NEIGH	1
#define KEYH_WIADJ	2
#define NKEYH		3

struct vmode {
	const char	*vm_name;
	int		 vm_clip;
	struct fvec	 vm_ndim[NGEOM];		/* node dimensions */
};

struct dmode {
	const char	*dm_name;
};

struct keyh {
	const char	 *kh_name;
	void		(*kh_spkeyh)(int, int, int);
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

void geom_setall(int);

void cursor_set(int);

void focus_cluster(struct fvec *);
void focus_selnodes(struct fvec *);

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
extern struct dmode	 dmodes[];
extern float		 clip;
extern struct fvec	 wi_repstart, wi_repdim;	/* repeat position & dim */
extern struct ivec	 widim;

extern int		 stereo_mode;
extern int		 window_ids[2];
extern int		 wid;
extern int		 spkey;
extern struct ivec	 mousev;
extern struct ivec	 winv;
extern struct keyh	 keyhtab[];
extern int		 keyh;

extern struct fvec	 focus;

#endif /* _ENV_H_ */
