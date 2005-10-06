/* $Id$ */

#include "compat.h"

#include <sys/queue.h>

#include "cdefs.h"
#include "mon.h"
#include "queue.h"

void
cocb_fps(__unused int a)
{
	fps = fps_cnt;
	fps_cnt = 0;
	glutTimerFunc(1000, cocb_fps, 0);
}

void
cocb_datasrc(__unused int a)
{
	st.st_rf |= RF_DATASRC | RF_CLUSTER;
	glutTimerFunc(5000, cocb_datasrc, 0);
}

void
cocb_clearstatus(__unused int a)
{
	status_clear();
	glutTimerFunc(5000, cocb_clearstatus, 0);
}

void
cocb_tourjob(__unused int a)
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
	glutTimerFunc(15000, cocb_tourjob, 0);
}
