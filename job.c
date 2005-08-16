/* $Id$ */

#include "cdefs.h"
#include "mon.h"

struct job *
job_findbyid(int id)
{
	int n, lo, hi, tid;

	lo = 0;
	hi = MAX(job_list.ol_tcur, job_list.ol_cur);
	while (lo <= hi) {
		n = MID(lo, hi);
		tid = job_list.ol_jobs[n]->j_id;
		if (tid < id)
			lo = n + 1;
		else if (tid > id)
			hi = n - 1;
		else
			return (job_list.ol_jobs[n]);
	}
	return (NULL);
}

void
job_highlight(struct job *j)
{

}
