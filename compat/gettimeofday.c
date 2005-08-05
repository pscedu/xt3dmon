/* $Id$ */

#include "compat.h"

int gettimeofday(struct timeval *tv, void *null)
{
	long tc;
	tc = GetTickCount();
	tv->tv_sec = tc * 1e6;
	tv->tv_usec = tc;
}
