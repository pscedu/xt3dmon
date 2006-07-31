/* $Id$ */

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

/* Node status classes. */
#define SC_FREE		0
#define SC_DISABLED	1
#define SC_DOWN		2
#define SC_USED		3
#define SC_SVC		4
#define NSC		5

/* Temperature class constants. */
#define TEMP_MIN	18
#define TEMP_MAX	73
#define NTEMPC		14

/* Failure class constants. */
#define FAIL_MIN	0
#define FAIL_MAX	20
#define NFAILC		6

/* Route error class constants. */
#define NRTC		10
#define RTC_SND		11
#define RTC_RCV		12

/* Seastar class constants. */
#define NSSC		10

/* Lustre class constants. */
#define NLUSTC		3

#define NODECLASS(val, max) \
	((max) ? 10 * ((val) - 1e-5) / (max) : 9)

struct nodeclass {
	char		*nc_name;
	struct fill	 nc_fill;
	int		 nc_nmemb;
};

extern struct nodeclass	 statusclass[];
extern struct nodeclass	 tempclass[NTEMPC];	/* XXX SPOT */
extern struct nodeclass	 failclass[NFAILC];	/* XXX SPOT */
extern struct nodeclass	 rtclass[NRTC];		/* XXX SPOT */
extern struct nodeclass	 rtpipeclass[NRTC];	/* XXX SPOT */
extern struct nodeclass	 ssclass[NSSC];		/* XXX SPOT */
extern struct nodeclass	 lustreclass[];
