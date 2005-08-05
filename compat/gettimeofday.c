/* $Id$ */

#include "compat.h"
#include "cdefs.h"

int
gettimeofday(struct timeval *tv, __unused void *null)
{
	long tc;
	tc = GetTickCount();
	tv->tv_sec = (int)(tc / 1e3);
	tv->tv_usec = tc - tv->tv_sec * 1e3;

	return 0;
}
