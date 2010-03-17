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

#include "objlist.h"
#include "fill.h"

#define YFL_CMD		100

struct yod {
	struct objhdr	 y_oh;
	int		 y_id;
	struct fill	 y_fill;

	int		 y_partid;
	char		 y_cmd[YFL_CMD];
	int		 y_ncpus;
	int		 y_single;
};

void		 yod_init(struct yod *);
struct yod	*yod_findbyid(int, int *);
int		 yod_cmp(const void *, const void *);
