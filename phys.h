/* $Id$ */

#include "queue.h"
#include "xmath.h"

struct physdim {
	char			*pd_name;
	size_t			 pd_mag;
	double			 pd_space;
	struct physdim		*pd_contains;
	struct physdim		*pd_containedby;
	int			 pd_spans;
	double			 pd_offset;
	struct fvec		 pd_size;
	int			 pd_flags;
	LIST_ENTRY(physdim)	 pd_link;
};

#define PDF_SKEL (1<<0)

LIST_HEAD(physdim_hd, physdim);

extern struct physdim_hd physdims;
