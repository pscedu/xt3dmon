/* $Id$ */

#include "mon.h"

struct job *
job_findbyid(int id)
{
	size_t n, lo, hi;

	lo = 0;
	hi = job_list.ol_cur;
	while (lo <= hi) {
		n = MID(lo, hi);
		if (n < id)
			lo = n + 1;
		else if (n > id)
			hi = n - 1;
		else
			return (job_list.ol_jobs[n]);
	}
	return (NULL);
}
