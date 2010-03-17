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

struct lnseg {
	struct fvec		ln_sv;
	struct fvec		ln_ev;
	SLIST_ENTRY(lnseg)	ln_link;
};

SLIST_HEAD(lnseghd, lnseg);

void lnseg_add(struct fvec *, struct fvec *);
void lnseg_clear(void);
void lnseg_draw(void);

extern struct lnseghd lnsegs;
