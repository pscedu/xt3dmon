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

#define PDF_SKEL (1 << 0)			/* render skeleton if OP_SKEL */

struct physcoord {				/* XXX: become just dynamic array */
	int			 pc_part;	/* partition */
	int			 pc_rack;	/* rackmount cabinet */
	int			 pc_iru;	/* individual rack unit */
	int			 pc_blade;
	int			 pc_node;
};

extern struct physdim	*physdim_top;

#endif /* _PHYS_H_ */
