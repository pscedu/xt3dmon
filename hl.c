/* $Id$ */

#include "compat.h"
#include "mon.h"

#define JST_HL_INV	(-1)
#define JST_HL_SELJOBS	(-2)

int hl_jstate = JST_HL_INV;

void
hl_refresh(void)
{
	if (hl_jstate >= 0 &&
	    hl_jstate < NJST)
		hl_state(hl_jstate);
	if (hl_jstate == JST_HL_SELJOBS)
		hl_seljobs();
}

void
hl_clearall(void)
{
	size_t j;

	for (j = 0; j < job_list.ol_cur; j++)
		job_list.ol_jobs[j]->j_fill.f_a = 0.0f;
	for (j = 0; j < NJST; j++)
		jstates[j].js_fill.f_a = 0.0f;
}

void
hl_restoreall(void)
{
	size_t j;

	hl_jstate = JST_HL_INV;
	for (j = 0; j < job_list.ol_cur; j++)
		job_list.ol_jobs[j]->j_fill.f_a = 1.0f;
	for (j = 0; j < NJST; j++)
		jstates[j].js_fill.f_a = 1.0f;
}

void
hl_state(int jst)
{
	hl_clearall();
	jstates[jst].js_fill.f_a = 1.0f;
	hl_jstate = JST_HL_INV;
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
}
