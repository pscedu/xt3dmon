/* $Id$ */

#include <time.h>

struct tm *
gmtime_r(const time_t *t, struct tm *tm)
{
	/* XXX multithreading */
	*tm = *gmtime(t);

	return (tm);
}
