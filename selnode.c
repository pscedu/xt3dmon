/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdlib.h>

#include "queue.h"
#include "node.h"
#include "selnode.h"
#include "flyby.h"
#include "state.h"

size_t nselnodes;
struct selnodes selnodes;

void
sn_replace(struct selnode *sn, struct node *n)
{
	if (sn->sn_nodep == n)
		return;
	sn->sn_nodep = n;
	if (flyby_mode == FBM_REC) {
		flyby_writeselnode(sn->sn_nodep->n_nid);
		flyby_writeselnode(n->n_nid);
	}
	st.st_rf |= RF_SELNODE;
}

/*
 * "Low-level" add: does the nitty-gritty of the data structure for
 * adding a specific node to the list of selected nodes.
 */
void
sn_insert(struct node *n)
{
	struct selnode *sn;

	if ((sn = malloc(sizeof(*sn))) == NULL)
		err(1, "malloc");
	sn->sn_nodep = n;
	SLIST_INSERT_HEAD(&selnodes, sn, sn_next);
	if (flyby_mode == FBM_REC)
		flyby_writeselnode(n->n_nid);
#if 0
	switch (st.st_mode) {
	case SM_JOBS:
		if (n->n_state == JST_USED)
			n->n_job->j_oh.oh_flags |= OHF_SEL;
		break;
	case SM_TEMP:
		break;
	case SM_FAIL:
		break;
	}
#endif
	st.st_rf |= RF_SELNODE;
	nselnodes++;
}

/*
 * "High-level" add: for commands such as "Select the node 415."
 */
void
sn_add(struct node *n)
{
	struct selnode *sn;

	SLIST_FOREACH(sn, &selnodes, sn_next)
		if (sn->sn_nodep == n)
			return;
	sn_insert(n);
}

void
sn_clear(void)
{
	struct selnode *sn, *next;

	if (SLIST_EMPTY(&selnodes))
		return;

	for (sn = SLIST_FIRST(&selnodes);
	    sn != SLIST_END(&selnodes); sn = next) {
		next = SLIST_NEXT(sn, sn_next);
		if (flyby_mode == FBM_REC)
			flyby_writeselnode(sn->sn_nodep->n_nid);
		free(sn);
	}
	SLIST_INIT(&selnodes);
	st.st_rf |= RF_SELNODE;
	nselnodes = 0;
}

int
sn_del(struct node *n)
{
	struct selnode *sn, **prev;

	SLIST_FOREACH_PREVPTR(sn, prev, &selnodes, sn_next)
		if (sn->sn_nodep == n) {
			*prev = SLIST_NEXT(sn, sn_next);
			free(sn);
			if (flyby_mode == FBM_REC)
				flyby_writeselnode(n->n_nid);
			st.st_rf |= RF_SELNODE;
			nselnodes--;
			return (1);
		}
	return (0);
}

void
sn_toggle(struct node *n)
{
	if (!sn_del(n))
		sn_insert(n);
}

void
sn_set(struct node *n)
{
	sn_clear();
	sn_insert(n);
}
