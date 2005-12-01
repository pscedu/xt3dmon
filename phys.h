/* $Id$ */

#include <sys/queue.h>

#include "queue.h"
#include "mon.h"

#define DIM_X 0
#define DIM_Y 1
#define DIM_Z 2

struct physdim {
	char			*pd_name;
	size_t			 pd_mag;
	double			 pd_space;
	struct physdim		*pd_contains;
	int			 pd_spans;
	double			 pd_offset;
	struct fvec		 pd_size;
	LIST_ENTRY(physdim)	 pd_link;
};

LIST_HEAD(physdim_hd, physdim);

extern struct physdim_hd physdims;
