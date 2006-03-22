/* $Id$ */

#include "fill.h"

#define SC_FREE		0
#define SC_DISABLED	1
#define SC_DOWN		2
#define SC_USED		3
#define SC_SVC		4
#define NSC		5

#define TEMP_MIN	18
#define TEMP_MAX	80
#define TEMP_NTEMPS	(sizeof(tempclass) / sizeof(tempclass[0]))

#define FAIL_MIN	0
#define FAIL_MAX	20
#define FAIL_NFAILS	(sizeof(failclass) / sizeof(failclass[0]))

/* Node highlighting. */
#define SC_HL_ALL	(-1)
#define SC_HL_NONE	(-2)
#define SC_HL_SELJOBS	(-3)

struct nodeclass {
	char		*nc_name;
	struct fill	 nc_fill;
};

extern struct nodeclass	 statusclass[];
extern struct nodeclass	 tempclass[14];		/* XXX */
extern struct nodeclass	 failclass[6];		/* XXX */

extern int hlsc;
