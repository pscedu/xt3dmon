/* $Id$ */

#include "mon.h"

#include "flyby.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "queue.h"
#include "selnode.h"

int hlsc = SC_HL_ALL;

void
hl_refresh(void)
{
	switch (hlsc) {
	case SC_HL_ALL:
		hl_restoreall();
		break;
	case SC_HL_NONE:
		hl_clearall();
		break;
	case SC_HL_SELJOBS:
		hl_seljobs();
		break;
	default:
		if (hlsc >= 0 && hlsc < NSC)
			hl_state(hlsc);
		break;
	}
}

void
hl_clearall(void)
{
	size_t j;

	hlsc = SC_HL_NONE;
	for (j = 0; j < job_list.ol_cur; j++)
		job_list.ol_jobs[j]->j_fill.f_a = 0.0f;
	for (j = 0; j < NSC; j++)
		statusclass[j].nc_fill.f_a = 0.0f;
	if (flyby_mode == FBM_REC)
		flyby_writehlsc(hlsc);
}

void
hl_restoreall(void)
{
	size_t j;

	hlsc = SC_HL_ALL;
	for (j = 0; j < job_list.ol_cur; j++)
		job_list.ol_jobs[j]->j_fill.f_a = 1.0f;
	for (j = 0; j < NSC; j++)
		statusclass[j].nc_fill.f_a = 1.0f;
	if (flyby_mode == FBM_REC)
		flyby_writehlsc(hlsc);
}

void
hl_state(int sc)
{
	hl_clearall();
	statusclass[sc].nc_fill.f_a = 1.0f;
	hlsc = sc;
	if (flyby_mode == FBM_REC)
		flyby_writehlsc(hlsc);
}

void
hl_seljobs(void)
{
	struct selnode *sn;
	int foundjob = 0;

	SLIST_FOREACH(sn, &selnodes, sn_next)
		if (sn->sn_nodep->n_job != NULL) {
			foundjob = 1;
			break;
		}
	if (!foundjob)
		return;

	hl_clearall();
	SLIST_FOREACH(sn, &selnodes, sn_next)
		if (sn->sn_nodep->n_job != NULL)
			job_hl(sn->sn_nodep->n_job);
	hlsc = SC_HL_SELJOBS;
	if (flyby_mode == FBM_REC)
		flyby_writehlsc(hlsc);
}
