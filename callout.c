/* $Id$ */

#include <sys/queue.h>

#include "mon.h"
#include "queue.h"

/*
 * The callout queue is a list containing elements
 * that include when they need to be called out.
 * The value of 'ce_when' for a given entry is
 * seconds relative to both the current time and
 * the 'ce_when' of entries in the list before it.
 *
 * The only design quirk is that entries cannot be
 * inserted before the first entry at any given
 * time.  If any are, they be processed at the time
 * in which the entry they have preceded was queued
 * for.  This is fine because, at any given time,
 * there is always a one-second entry in the queue
 * (FPS), and the resolution is limited to seconds.
 */

struct calloutq calloutq;

void
callout_add(int when, void (*cb)(void))
{
	struct callout_ent *ce, *nce;

	if ((nce = malloc(sizeof(*nce))) == NULL)
		err(1, NULL);
	nce->ce_cb = cb;
	nce->ce_when = when;

	LIST_FOREACH(ce, &calloutq, ce_link) {
		if (nce->ce_when > ce->ce_when)
			nce->ce_when -= ce->ce_when;
		else {
			LIST_INSERT_BEFORE(ce, nce, ce_link);
			ce->ce_when -= nce->ce_when;
			return;
		}
	}
	/* No elements; insert new at head. */
	LIST_INSERT_HEAD(&calloutq, nce, ce_link);
}

__inline int
callout_next(int elapsed)
{
	struct callout_ent *ce;
	int when;

	ce = LIST_FIRST(&calloutq);
	if (ce == NULL || elapsed < ce->ce_when)
		return (0);
	ce->ce_cb();
	when = ce->ce_when - elapsed;
	LIST_REMOVE(ce, ce_link);
	free(ce);

	ce = LIST_FIRST(&calloutq);
	if (ce)
		ce->ce_when -= when;
	return (1);
}

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
			job_goto(job_list.ol_jobs[j]->j_id);
			break;
		}
	callout_add(10, cocb_tourjob);
}
