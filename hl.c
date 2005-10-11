/* $Id$ */

#include "compat.h"
#include "mon.h"

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
}
