/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "flyby.h"
#include "node.h"
#include "panel.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"

size_t nselnodes;
struct selnodes selnodes;

void
sn_replace(struct selnode *sn, struct node *n)
{
	if (sn->sn_nodep == n)
		return;
	sn->sn_nodep->n_flags &= ~NF_SELNODE;
	sn->sn_nodep = n;
	n->n_flags |= NF_SELNODE;

	if (flyby_mode == FBM_REC) {
		flyby_writeselnode(sn->sn_nodep->n_nid, &sn->sn_offv);
		flyby_writeselnode(n->n_nid, &fv_zero);
	}
	st.st_rf |= RF_SELNODE;
	if (st.st_vmode == VM_VNEIGHBOR)
		st.st_rf |= RF_VMODE;
	selnode_clean = 0;
}

/*
 * "Low-level" add: does the nitty-gritty of the data structure for
 * adding a specific node to the list of selected nodes.
 */
static void
sn_insert(struct node *n, const struct fvec *offv)
{
	struct selnode *sn;

	n->n_flags |= NF_SELNODE;
	if ((sn = malloc(sizeof(*sn))) == NULL)
		err(1, "malloc");
	sn->sn_nodep = n;
	sn->sn_offv = *offv;
	SLIST_INSERT_HEAD(&selnodes, sn, sn_next);

	if (flyby_mode == FBM_REC)
		flyby_writeselnode(n->n_nid, offv);
	st.st_rf |= RF_SELNODE;
	if (st.st_vmode == VM_VNEIGHBOR)
		st.st_rf |= RF_VMODE;
	nselnodes++;
	if (flyby_mode != FBM_PLAY)
		panel_show(PANEL_NINFO);
	selnode_clean = 0;
}

/*
 * "High-level" add: for commands such as "Select the node 415."
 */
void
sn_add(struct node *n, const struct fvec *offv)
{
	struct selnode *sn;

	SLIST_FOREACH(sn, &selnodes, sn_next)
		if (sn->sn_nodep == n)
			return;
	sn_insert(n, offv);
}

void
sn_clear(void)
{
	struct selnode *sn, *next;

	if (SLIST_EMPTY(&selnodes))
		return;

	for (sn = SLIST_FIRST(&selnodes);
	    sn != SLIST_END(&selnodes); sn = next) {
		sn->sn_nodep->n_flags &= ~NF_SELNODE;
		next = SLIST_NEXT(sn, sn_next);
		if (flyby_mode == FBM_REC)
			flyby_writeselnode(sn->sn_nodep->n_nid, &fv_zero);
		free(sn);
	}
	SLIST_INIT(&selnodes);

	st.st_rf |= RF_SELNODE;
	if (st.st_vmode == VM_VNEIGHBOR)
		st.st_rf |= RF_VMODE;
	nselnodes = 0;
	if (flyby_mode != FBM_PLAY)
		panel_hide(PANEL_NINFO);
	selnode_clean = 0;
}

int
sn_del(struct node *n)
{
	struct selnode *sn, **prev;

	SLIST_FOREACH_PREVPTR(sn, prev, &selnodes, sn_next)
		if (sn->sn_nodep == n) {
			n->n_flags &= ~NF_SELNODE;
			*prev = SLIST_NEXT(sn, sn_next);
			free(sn);

			if (flyby_mode == FBM_REC)
				flyby_writeselnode(n->n_nid, &fv_zero);
			st.st_rf |= RF_SELNODE;
			if (st.st_vmode == VM_VNEIGHBOR)
				st.st_rf |= RF_VMODE;
			if (--nselnodes == 0 && flyby_mode != FBM_PLAY)
				panel_hide(PANEL_NINFO);
			selnode_clean = 0;
			return (1);
		}
	return (0);
}

void
sn_toggle(struct node *n, const struct fvec *offv)
{
	if (!sn_del(n))
		sn_insert(n, offv);
}

void
sn_set(struct node *n, const struct fvec *offv)
{
	sn_clear();
	sn_insert(n, offv);
}

void
sn_addallvis(void)
{
	struct node *n, **np;

	NODE_FOREACH_PHYS(n, np)
		if (node_show(n))
			sn_add(n, &fv_zero);
}
