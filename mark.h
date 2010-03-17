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

#include "queue.h"
#include "xmath.h"

struct mark {
	struct fvec		 m_fv;
	struct fill		*m_fp;
	SLIST_ENTRY(mark)	 m_link;
};

SLIST_HEAD(markhd, mark);

void mark_add(struct fvec *, struct fill *);
void mark_clear(void);
void mark_draw(void);

extern struct markhd marks;
