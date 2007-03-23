/* $Id$ */

#ifndef _PHYS_H_
#define _PHYS_H_

#include "queue.h"
#include "xmath.h"

struct physdim {
	char			*pd_name;
	int			 pd_mag;
	double			 pd_space;
	struct physdim		*pd_contains;
	struct physdim		*pd_containedby;
	int			 pd_spans;
	struct fvec		 pd_offset;
	struct fvec		 pd_size;
	int			 pd_flags;

	LIST_ENTRY(physdim)	 pd_link;	/* used only in construction */
};

#define PDF_SKEL (1<<0)

extern struct physdim	*physdim_top;

#endif /* _PHYS_H_ */
