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

int
sel_del(struct node *n)
{
	struct selnode *sn, **prev;

	SLIST_FOREACH_PREVPTR(sn, prev, &selnodes, sn_next)
		if (sn->sn_nodep == n) {
			SLIST_REMOVE_AFTER(*prev, sn_next);
			if (flyby_mode == FBM_REC)
				flyby_writeselnode(n->n_nid);
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
