/* $Id$ */

#include "mon.h"

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
