/* $Id$ */

#include "mon.h"
#include "cdefs.h"

struct job *
job_findbyid(int id)
{
	size_t n;

	for (n = 0; n < job_list.ol_cur; n++)
		if (job_list.ol_jobs[n]->j_id == id)
			return (job_list.ol_jobs[n]);
	return (NULL);
}

#if 0
struct job *
job_findbyid(size_t id)
{
	size_t n, tid;
	int lo, hi;

	lo = 0;
	hi = MAX(job_list.ol_tcur, job_list.ol_cur);
	if (hi == 0)
		return (NULL);
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
#endif

__inline void
job_hl(struct job *j)
{
	j->j_fill.f_a = 1.0f;
}

void
job_goto(__unused struct job *j)
{
}

void
prjobs(void)
{
	size_t i;

	printf(" => JOBS [max %d]", job_list.ol_max);
	for (i = 0; i < job_list.ol_tcur; i++)
		printf(" %d", job_list.ol_jobs[i]->j_id);
	printf(" ||");
	for (; i < job_list.ol_max; i++)
		printf(" %d", job_list.ol_jobs[i]->j_id);
	printf("\n");
}
