/* $Id$ */

#include "mon.h"

struct job *
job_findbyid(int id)
{
	size_t n;
	
	for (n = 0; n < job_list.ol_cur; n++)
		if (job_list.ol_jobs[n]->j_id == id)
			return (job_list.ol_jobs[n]);
	return (NULL);
}
