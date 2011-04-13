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

#include "cdefs.h"
#include "env.h"
#include "fill.h"
#include "mach.h"
#include "node.h"
#include "state.h"
#include "tween.h"
#include "xmath.h"

struct node	 *nodes;
struct node	**node_nidmap;		/* node ID (nid) lookup to node pointer */
struct node	**node_wimap;		/* X/Y/Z lookup to node pointer */
int		  node_wimap_len;
struct ivec	  widim;		/* wired dimension bounds */

/*
 * Determine row/column within module.
 *
 *	  +---+ +---+	Row 1	^ y
 *	  | 2 | | 3 |		|
 *	  +---+ +---+		|
 *	+---+ +---+	Row 0	|
 *	| 1 | | 0 |		|
 *	+---+ +---+		|
 *				|
 *	Col 1 Col 0		|
 *				|
 * z <--------------------------+ (0,0,0)
 */
__inline void
node_getmodpos(int n, int *row, int *col)
{
	*row = n / (NNODES / 2);
	*col = (n & 1) ^ ((n & 2) >> 1);
}

__inline void
node_adjmodpos(int n, struct fvec *fv)
{
	int row, col;

	node_getmodpos(n, &row, &col);
	fv->fv_y += row * (NODESPACE + NODEHEIGHT);
	fv->fv_z += col * (NODESPACE + NODEDEPTH) + row * NODESHIFT;
}

void
node_setphyspos(struct node *n, struct fvec *fv)
{
#if 0
	struct physdim *pd;
	int i, pos;

	fv->fv_x = 0.0;
	fv->fv_y = 0.0;
	fv->fv_z = 0.0;

	i = 0;
	LIST_FOREACH(pd, &physdim, pd_link) {
		pos = n->n_physcoord[i++];
		switch (pd->pd_spans) {
		case DIM_X:
			fv->fv_x += ;
			break;
		case DIM_Y:
			fv->fv_y += ;
			break;
		case DIM_Z:
			fv->fv_z += ;
			break;
		}
	}
#endif
	struct physcoord pc;

	node_physpos(n, &pc);
	fv->fv_x = NODESPACE + pc.pc_rack * (CABWIDTH + CABSPACE) +
	    pc.pc_blade * (MODWIDTH + MODSPACE);
	fv->fv_y = NODESPACE + pc.pc_iru * (CAGEHEIGHT + CAGESPACE);
	fv->fv_z = NODESPACE + pc.pc_row * (ROWDEPTH + ROWSPACE);
	node_adjmodpos(pc.pc_node, fv);
}

void
node_physpos(struct node *node, struct physcoord *pc)
{
	int pos;

	pos = node - nodes;

	pc->pc_row = pos / (NRACKS * NIRUS * NBLADES * NNODES);
	pos %= NRACKS * NIRUS * NBLADES * NNODES;
	pc->pc_rack = pos / (NIRUS * NBLADES * NNODES);
	pos %= NIRUS * NBLADES * NNODES;
	pc->pc_iru = pos / (NBLADES * NNODES);
	pos %= NBLADES * NNODES;
	pc->pc_blade = pos / NNODES;
	pos %= NNODES;
	pc->pc_node = pos;
}

struct node *
node_neighbor(int vm, struct node *n, int rd, int *flip)
{
	int row, col, adj, dim;
	struct physcoord pc;
	struct node *ng;
	struct ivec iv;

	adj = 1;
	switch (rd) {
	case RD_NEGX:
	case RD_NEGY:
	case RD_NEGZ:
		adj = -1;
		break;
	}

	switch (rd) {
	case RD_NEGX:
	case RD_POSX:
		dim = DIM_X;
		break;
	case RD_NEGY:
	case RD_POSY:
		dim = DIM_Y;
		break;
	case RD_NEGZ:
	case RD_POSZ:
		dim = DIM_Z;
		break;
	default:
		err(1, "unknown relative dir: %d", rd);
	}

	if (flip)
		*flip = 0;

	iv = n->n_wiv;

	ng = NULL; /* gcc */
	switch (vm) {
	case VM_WIRED:
	case VM_WIONE:
	case VM_VNEIGHBOR:
		do {
			iv.iv_val[dim] = negmod(iv.iv_val[dim] + adj,
			    widim.iv_val[dim]);
			ng = NODE_WIMAP(iv.iv_x, iv.iv_y, iv.iv_z);

			if (flip &&
			    ((adj > 0 && iv.iv_val[dim] == 0) ||
			     (adj < 0 && iv.iv_val[dim] == widim.iv_val[dim] - 1)))
				*flip = adj;
		} while (ng == NULL);
		break;
	case VM_PHYS:
		node_physpos(n, &pc);
		do {
			switch (rd) {
			case RD_POSZ:
			case RD_NEGZ:
				pc.pc_blade += adj;
				if (pc.pc_blade < 0) {
					pc.pc_rack--;
					pc.pc_blade += NBLADES;
					pc.pc_rack += NRACKS;
				} else if (pc.pc_blade >= NBLADES)
					pc.pc_rack++;
				pc.pc_blade %= NBLADES;
				pc.pc_rack %= NRACKS;
				break;
			case RD_POSY:
			case RD_NEGY:
				/*
				 * Move our position within a blade.
				 *
				 *	n | need
				 *	=========
				 *	0 | 3
				 *	3 | 0
				 *	2 | 1
				 *	1 | 2
				 */
				if (pc.pc_node % 2)
					pc.pc_node++;
				else
					pc.pc_node--;
				if (pc.pc_node < 0)
					pc.pc_node += NNODES;
				pc.pc_node %= NNODES;
				node_getmodpos(pc.pc_node, &row, &col);
				if (adj == -1) {
					/*
					 * If we ended up in the top
					 * portion when traveling
					 * down, we wrapped.
					 */
					if (row == 1) {
						pc.pc_iru--;
						pc.pc_iru += NIRUS;
					}
				} else {
					/*
					 * If we ended up in the
					 * bottom portion, we wrapped.
					 */
					if (row == 0)
						pc.pc_iru++;
				}
				pc.pc_iru %= NIRUS;
				break;
			case RD_POSX:
			case RD_NEGX:
				/*
				 * Move our position within a blade.
				 *
				 *	n | need
				 *	=========
				 *	0 | 1
				 *	1 | 0
				 *	2 | 3
				 *	3 | 2
				 */
				if (pc.pc_node % 2)
					pc.pc_node--;
				else
					pc.pc_node++;
				if (pc.pc_node < 0)
					pc.pc_node += NNODES;
				pc.pc_node %= NNODES;
				node_getmodpos(pc.pc_node, &row, &col);
				if (adj == -1) {
					/*
					 * If we ended up in the
					 * front column when moving
					 * back, we wrapped.
					 */
					if (col == 1) {
						pc.pc_row--;
						pc.pc_row += NROWS;
					}
				} else {
					/*
					 * If we ended up in the
					 * back column when moving
					 * forward, we wrapped.
					 */
					if (col == 0)
						pc.pc_row++;
				}
				pc.pc_row %= NROWS;
				break;
			}
			ng = node_for_pc(&pc);
		} while ((ng->n_flags & NF_VALID) == 0);
		break;
	}
	return (ng);
}

struct node *
node_for_pc(const struct physcoord *pc)
{
	return (&nodes[
	    pc->pc_row * (NNODES * NBLADES * NIRUS * NRACKS) +
	    pc->pc_rack * (NNODES * NBLADES * NIRUS) +
	    pc->pc_iru * (NNODES * NBLADES) +
	    pc->pc_blade * (NNODES) +
	    pc->pc_node]);
}

struct node *
node_for_nid(int nid)
{
	if (nid > machine.m_nidmax || nid < 0)
		return (NULL);
	return (node_nidmap[nid]);
}

#define GOTO_DIST_PHYS 2.5
#define GOTO_DIST_LOG  2.5

void
node_goto(struct node *n)
{
	struct physcoord pc;
	int row, col;

	tween_push();
	st.st_v = n->n_v;
	switch (st.st_vmode) {
	case VM_PHYS:
		st.st_ur = 0.0;
		st.st_urev = 0;

		st.st_lv.fv_x = 0.0f;
		st.st_lv.fv_y = 0.0f;
		st.st_v.fv_y += 0.5 * NODEHEIGHT;

		node_physpos(n, &pc);
		node_getmodpos(pc.pc_node, &row, &col);
		/* Right side (positive z). */
		if (row == 1) {
			st.st_v.fv_z += NODEDEPTH + GOTO_DIST_PHYS;
			st.st_lv.fv_z = -1.0;
		} else {
			st.st_v.fv_z -= GOTO_DIST_PHYS;
			st.st_lv.fv_z = 1.0;
		}
		break;
	case VM_WIRED:
	case VM_WIONE:
	case VM_VNEIGHBOR:
		/* Set to the front where the label is. */
		st.st_v.fv_x -= GOTO_DIST_LOG;
		st.st_v.fv_y += 0.5 * n->n_dimp->fv_h;
		st.st_v.fv_z += 0.5 * n->n_dimp->fv_w;

		vec_set(&st.st_lv, 1.0, 0.0, 0.0);
		st.st_ur = 0.0;
		st.st_urev = 0;
		break;
	}
	tween_pop();
}

__inline int
node_show(const struct node *n)
{
return (1);
printf("hi\n");
	if ((st.st_opts & OP_SUBSET) && (n->n_flags & NF_SUBSET) == 0)
		return (0);
	if (ATTR_TESTALL(n->n_flags, NF_VALID | NF_VMVIS) &&
	    n->n_fillp->f_a > 0.01f)
		return (1);
	return (0);
}

void
node_center(const struct node *n, struct fvec *fvp)
{
	*fvp = n->n_v;
	fvp->fv_x += n->n_dimp->fv_w / 2.0;
	fvp->fv_y += n->n_dimp->fv_h / 2.0;
	fvp->fv_z += n->n_dimp->fv_d / 2.0;
}

int
node_wineighbor_nhops(const struct node *a, const struct node *b)
{
	int nhops, i, step;

	nhops = 0;
	for (i = 0; i < NDIM; i++) {
		step = abs(a->n_wiv.iv_val[i] - b->n_wiv.iv_val[i]);

		/*
		 * If we are travelling further than half way,
		 * go to a different iteration of the cube, which
		 * will be shorter.
		 */
		if (step > widim.iv_val[i] / 2)
			nhops += abs(widim.iv_val[i] - step);
		else
			nhops += step;
	}
	return (nhops);
}
