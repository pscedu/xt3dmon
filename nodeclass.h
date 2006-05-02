/* $Id$ */

#include "fill.h"

#define SC_FREE		0
#define SC_DISABLED	1
#define SC_DOWN		2
#define SC_USED		3
#define SC_SVC		4
#define NSC		5

#define TEMP_MIN	18
#define TEMP_MAX	73
#define NTEMPC		14

#define FAIL_MIN	0
#define FAIL_MAX	20
#define NFAILC		6

#define NRTC		10

struct nodeclass {
	char		*nc_name;
	struct fill	 nc_fill;
	int		 nc_nmemb;
};

extern struct nodeclass	 statusclass[];
extern struct nodeclass	 tempclass[NTEMPC];	/* XXX SPOT */
extern struct nodeclass	 failclass[NFAILC];	/* XXX SPOT */
extern struct nodeclass	 rtclass[NRTC];		/* XXX SPOT */
