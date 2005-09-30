/* $Id$ */

#include <sys/queue.h>

#include "mon.h"
#include "queue.h"

void
cocb_fps(void)
{
	static int fcnt;

	fps = fcnt;
	fcnt = 0;

	/* Call us again one second in the future. */
	callout_add(1, cocb_fps);
}

void
cocb_datasrc(void)
{
	st.st_rf |= RF_DATASRC | RF_CLUSTER;
	callout_add(5, cocb_datasrc);
}

void
cocb_clearstatus(void)
{
	status_clear();
	callout_add(5, cocb_clearstatus);
}

void
cocb_tourjob(void)
{
	size_t j;

	if ((st.st_opts & OP_TOURJOB) == 0)
		return;
	for (j = 0; j < job_list.ol_cur; j++)
		if ((job_list.ol_jobs[j]->j_oh.oh_flags & JOHF_TOURED) == 0) {
			job_list.ol_jobs[j]->j_oh.oh_flags |= JOHF_TOURED;
			job_goto(job_list.ol_jobs[j]);
			break;
		}
	callout_add(10, cocb_tourjob);
}
