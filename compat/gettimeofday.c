/* $Id$ */

#include "compat.h"
#include "cdefs.h"

int
gettimeofday(struct timeval *tv, __unused void *null)
{
	long tc;
	tc = GetTickCount();
	tv->tv_sec = tc * 1e6;
	tv->tv_usec = tc;
}
