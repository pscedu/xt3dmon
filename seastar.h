/* $Id$ */

#include "mon.h"

#define NVC		4

#define SSCNT_NBLK	0
#define SSCNT_NFLT	1
#define SSCNT_NPKT	2
#define NSSCNT		3

struct seastar {
	uint64_t ss_cnt[NSSCNT][NVC];
};

extern struct seastar ss_max;
