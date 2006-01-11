/* $Id$ */

#include <math.h>

#include "mon.h"

#define SQUARE(x)	((x) * (x))
#define SIGN(x)		((x) == 0 ? 1 : abs(x) / (x))

#define DEG_TO_RAD(x)	((x) * M_PI / 180)

#define SIGNF(a) \
	(a < 0.0f ? -1.0f : 1.0f)

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
