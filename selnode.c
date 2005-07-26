/* $Id$ */

#include <stdlib.h>

#include "queue.h"
#include "mon.h"

size_t nselnodes;

/*
 * "Low-level" add: does the nitty-gritty of the data structure for
 * adding a specific node to the list of selected nodes.
 */
void
sel_insert(struct node *n)
{
	struct selnode *sn;

	if ((sn = malloc(sizeof(*sn))) == NULL)
		err(1, "malloc");
	sn->sn_nodep = n;
	SLIST_INSERT_HEAD(&selnodes, sn, sn_next);
	if (flyby_mode == FBM_REC)
		flyby_writeselnode(n->n_nid);
	st.st_rf |= RF_SELNODE;
	nselnodes++;
}

/*
 * "High-level" add: for commands such as "Select the node 415."
 */
void
sel_add(struct node *n)
{
	struct selnode *sn;

	SLIST_FOREACH(sn, &selnodes, sn_next)
		if (sn->sn_nodep == n)
			return;
	sel_insert(n);
}

void
sel_clear(void)
{
	struct selnode *sn, *next;

	if (SLIST_EMPTY(&selnodes))
		return;

	for (sn = SLIST_FIRST(&selnodes);
	    sn != SLIST_END(&selnodes); sn = next) {
		next = SLIST_NEXT(sn, sn_next);
		free(sn);
		if (flyby_mode == FBM_REC)
			flyby_writeselnode(sn->sn_nodep->n_nid);
	}
	SLIST_INIT(&selnodes);
	st.st_rf |= RF_SELNODE;
	nselnodes = 0;
}

int
sel_del(struct node *n)
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
sel_toggle(struct node *n)
{
	if (!sel_del(n))
		sel_insert(n);
}

void
sel_set(struct node *n)
{
	sel_clear();
	sel_insert(n);
}
