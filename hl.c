/* $Id$ */

#include "compat.h"
#include "mon.h"

int hl_jstate = JST_HL_ALL;

void
hl_refresh(void)
{
	switch (hl_jstate) {
	case JST_HL_ALL:
		hl_restoreall();
		break;
	case JST_HL_NONE:
		hl_clearall();
		break;
	case JST_HL_SELJOBS:
		hl_seljobs();
		break;
	default:
		if (hl_jstate >= 0 &&
		    hl_jstate < NJST)
			hl_state(hl_jstate);
		break;
	}
}

void
hl_clearall(void)
{
	size_t j;

	hl_jstate = JST_HL_NONE;
	for (j = 0; j < job_list.ol_cur; j++)
		job_list.ol_jobs[j]->j_fill.f_a = 0.0f;
	for (j = 0; j < NJST; j++)
		jstates[j].js_fill.f_a = 0.0f;
	if (flyby_mode == FBM_REC)
		flyby_writehljstate(hl_jstate);
}

void
hl_restoreall(void)
{
	size_t j;

	hl_jstate = JST_HL_ALL;
	for (j = 0; j < job_list.ol_cur; j++)
		job_list.ol_jobs[j]->j_fill.f_a = 1.0f;
	for (j = 0; j < NJST; j++)
		jstates[j].js_fill.f_a = 1.0f;
	if (flyby_mode == FBM_REC)
		flyby_writehljstate(hl_jstate);
}

void
hl_state(int jst)
{
	hl_clearall();
	jstates[jst].js_fill.f_a = 1.0f;
	hl_jstate = jst;
	if (flyby_mode == FBM_REC)
		flyby_writehljstate(hl_jstate);
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
	hl_jstate = JST_HL_SELJOBS;
	if (flyby_mode == FBM_REC)
		flyby_writehljstate(hl_jstate);
}
