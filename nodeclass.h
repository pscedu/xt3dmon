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

#include "fill.h"

/*
 * Generic/global node classes.
 * If the value of a nodeclass is something other than
 * these values, it is a value specific to the current
 * dmode.
 */
#define NC_ALL		(-1)
#define NC_NONE		(-2)
#define NC_SELDM	(-3)
#define NC_INVAL	(-4)

/* Node status classes. */
#define SC_FREE		0
#define SC_DISABLED	1
#define SC_DOWN		2
#define SC_USED		3
#define SC_SVC		4
#define SC_ADMDOWN	5
#define SC_ROUTE	6
#define SC_SUSPECT	7
#define SC_UNAVAIL	8
#define NSC		9

/* Temperature class constants. */
#define TEMP_MIN	18
#define TEMP_MAX	73
#define NTEMPC		14

/* Route error class constants. */
#define NRTC		10
#define RTC_SND		11
#define RTC_RCV		12

/* Seastar class constants. */
#define NSSC		10

/* Lustre class constants. */
#define NLUSTC		3

struct nodeclass {
	char		*nc_name;
	struct fill	 nc_fill;
	int		 nc_nmemb;
};

void		 nc_runall(void (*)(struct fill *));
void		 nc_runsn(void (*)(struct fill *));
int		 nc_apply(void (*)(struct fill *), size_t);
int		 nc_set(int);
struct fill	*nc_getfp(size_t);

extern struct nodeclass	 statusclass[];
extern struct nodeclass	 tempclass[NTEMPC];	/* XXX SPOT */
extern struct nodeclass	 rtclass[NRTC];		/* XXX SPOT */
extern struct nodeclass	 rtpipeclass[NRTC];	/* XXX SPOT */
extern struct nodeclass	 ssclass[NSSC];		/* XXX SPOT */
extern struct nodeclass	 lustreclass[];
