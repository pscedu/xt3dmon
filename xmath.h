/* $Id$ */

#ifndef _XMATH_H_
#define _XMATH_H_

#include <math.h>

#include "mon.h"

#define SQUARE(x)	((x) * (x))
#define SIGN(x)		(x < 0 ? -1 : 1)
#define SIGNF(a)	(a < 0.0f ? -1.0f : 1.0f)

#define DST(a, b)					\
	(sqrt(						\
	    SQUARE((a)->fv_x - (b)->fv_x) +		\
	    SQUARE((a)->fv_y - (b)->fv_y) +		\
	    SQUARE((a)->fv_z - (b)->fv_z)))

#define DEG_TO_RAD(x)	((x) * M_PI / 180.0)

#define IVEC_FOREACH(tiv, iv)				\
	for ((tiv)->iv_x = 0;				\
	     (tiv)->iv_x < (iv)->iv_w;			\
	     (tiv)->iv_x++)				\
		for ((tiv)->iv_y = 0;			\
		     (tiv)->iv_y < (iv)->iv_h;		\
		     (tiv)->iv_y++)			\
			for ((tiv)->iv_z = 0;		\
			     (tiv)->iv_z < (iv)->iv_d;	\
			     (tiv)->iv_z++)

struct fvec {
	float	fv_val[3];
#define fv_x fv_val[0]
#define fv_y fv_val[1]
#define fv_z fv_val[2]

#define fv_w fv_val[0]
#define fv_h fv_val[1]
#define fv_d fv_val[2]

#define fv_r fv_val[0]
#define fv_t fv_val[1]
#define fv_p fv_val[2]
};

struct ivec {
	int	iv_val[3];
#define iv_x iv_val[0]
#define iv_y iv_val[1]
#define iv_z iv_val[2]

#define iv_w iv_x
#define iv_h iv_y
#define iv_d iv_z

#define iv_u iv_x
#define iv_v iv_y
};

/* math.c */
int	 negmod(int, int);
double	 negmodf(double, double);

/* vec.c */
int	 ivec_eq(const struct ivec *, const struct ivec *);
void	 ivec_set(struct ivec *, int, int, int);

int	 vec_eq(const struct fvec *, const struct fvec *);
void	 vec_cart2sphere(const struct fvec *, struct fvec *);
void	 vec_sphere2cart(const struct fvec *, struct fvec *);
void	 vec_crossprod(struct fvec *, const struct fvec *, const struct fvec *);
void	 vec_normalize(struct fvec *);
float	 vec_mag(const struct fvec *);
void	 vec_set(struct fvec *, double, double, double);
void	 vec_copyto(const struct fvec *, struct fvec *);
void	 vec_addto(const struct fvec *, struct fvec *);
void	 vec_subfrom(const struct fvec *, struct fvec *);
void	 vec_sub(struct fvec *, const struct fvec *, const struct fvec *);
void	 vec_rotate(struct fvec *, const struct fvec *, double deg);

#endif /* _XMATH_H_ */
