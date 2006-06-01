/* $Id$ */

#include "mon.h"

#include <stdlib.h>

#include "env.h"
#include "fill.h"
#include "node.h"
#include "state.h"
#include "tween.h"
#include "xmath.h"

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
	fv->fv_x = NODESPACE + pc.pc_cb * (CABWIDTH + CABSPACE) +
	    pc.pc_m * (MODWIDTH + MODSPACE);
	fv->fv_y = NODESPACE + pc.pc_cg * (CAGEHEIGHT + CAGESPACE);
	fv->fv_z = NODESPACE + pc.pc_r * (ROWDEPTH + ROWSPACE);
	node_adjmodpos(pc.pc_n, fv);
}

void
node_physpos(struct node *node, struct physcoord *pc)
{
	int pos;

	pos = node - &nodes[0][0][0][0][0];

	pc->pc_r = pos / (NCABS * NCAGES * NMODS * NNODES);
	pos %= NCABS * NCAGES * NMODS * NNODES;
	pc->pc_cb = pos / (NCAGES * NMODS * NNODES);
	pos %= NCAGES * NMODS * NNODES;
	pc->pc_cg = pos / (NMODS * NNODES);
	pos %= NMODS * NNODES;
	pc->pc_m = pos / NNODES;
	pos %= NNODES;
	pc->pc_n = pos;
}

struct node *
node_neighbor(struct node *node, int amt, int dir)
{
	int rem, adj, row, col;
	struct physcoord pc;
	struct node *rn;
	struct ivec iv;

	adj = 1;
	rn = NULL;
	switch (dir) {
	case DIR_LEFT:
		dir = DIR_RIGHT;
		adj *= -1;
		break;
	case DIR_DOWN:
		dir = DIR_UP;
		adj *= -1;
		break;
	case DIR_BACK:
		dir = DIR_FORWARD;
		adj *= -1;
		break;
	}

	node_physpos(node, &pc);
	iv = node->n_wiv;
	for (rem = abs(amt); rem > 0; rem--) {
		switch (st.st_vmode) {
		case VM_PHYSICAL:
			switch (dir) {
			case DIR_RIGHT:
				pc.pc_m += adj;
				if (pc.pc_m < 0) {
					pc.pc_cb--;
					pc.pc_m += NMODS;
					pc.pc_cb += NCABS;
				} else if (pc.pc_m >= NMODS)
					pc.pc_cb++;
				pc.pc_m %= NMODS;
				pc.pc_cb %= NCABS;
				break;
			case DIR_UP:
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
				if (pc.pc_n % 2)
					pc.pc_n++;
				else
					pc.pc_n--;
				if (pc.pc_n < 0)
					pc.pc_n += NNODES;
				pc.pc_n %= NNODES;
				node_getmodpos(pc.pc_n, &row, &col);
				if (adj == -1) {
					/*
					 * If we ended up in the top
					 * portion when traveling
					 * down, we wrapped.
					 */
					if (row == 1) {
						pc.pc_cg--;
						pc.pc_cg += NCAGES;
					}
				} else {
					/*
					 * If we ended up in the
					 * bottom portion, we wrapped.
					 */
					if (row == 0)
						pc.pc_cg++;
				}
				pc.pc_cg %= NCAGES;
				break;
			case DIR_FORWARD:
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
				if (pc.pc_n % 2)
					pc.pc_n--;
				else
					pc.pc_n++;
				if (pc.pc_n < 0)
					pc.pc_n += NNODES;
				pc.pc_n %= NNODES;
				node_getmodpos(pc.pc_n, &row, &col);
				if (adj == -1) {
					/*
					 * If we ended up in the
					 * front column when moving
					 * back, we wrapped.
					 */
					if (col == 1) {
						pc.pc_r--;
						pc.pc_r += NROWS;
					}
				} else {
					/*
					 * If we ended up in the
					 * back column when moving
					 * forward, we wrapped.
					 */
					if (col == 0)
						pc.pc_r++;
				}
				pc.pc_r %= NROWS;
				break;
			}
			rn = &nodes[pc.pc_r][pc.pc_cb][pc.pc_cg][pc.pc_m][pc.pc_n];
			break;
		case VM_WIRED:
		case VM_WIREDONE:
			switch (dir) {
			case DIR_RIGHT:
				iv.iv_x += adj + widim.iv_w;
				iv.iv_x %= widim.iv_w;
				break;
			case DIR_UP:
				iv.iv_y += adj + widim.iv_h;
				iv.iv_y %= widim.iv_h;
				break;
			case DIR_FORWARD:
				iv.iv_z += adj + widim.iv_d;
				iv.iv_z %= widim.iv_d;
				break;
			}
			rn = wimap[iv.iv_x][iv.iv_y][iv.iv_z];
			break;
		}
		if (rn->n_flags & NF_HIDE)
			rem++;
	}
	return (rn);
}

struct node *
node_for_nid(int nid)
{
	if (nid > NID_MAX || nid < 0)
		return (NULL);
	return (invmap[nid]);
}

#define GOTO_DIST_PHYS 2.5
#define GOTO_DIST_LOG  2.5

void
node_goto(struct node *n)
{
	struct physcoord pc;
	int row, col;

	tween_push(TWF_LOOK | TWF_POS);
	st.st_v = *n->n_v;
	switch (st.st_vmode) {
	case VM_PHYSICAL:
		st.st_lx = 0.0f;
		st.st_ly = 0.0f;

		st.st_y += 0.5 * NODEHEIGHT;

		node_physpos(n, &pc);
		node_getmodpos(pc.pc_n, &row, &col);
		/* Right side (positive z). */
		if (row == 1) {
			st.st_z += NODEDEPTH + GOTO_DIST_PHYS;
			st.st_lz = -1.0;
		} else {
			st.st_z -= GOTO_DIST_PHYS;
			st.st_lz = 1.0;
		}
		break;
	case VM_WIRED:
	case VM_WIREDONE:
		/* Set to the front where the label is. */
		st.st_x -= GOTO_DIST_LOG;
		st.st_y += 0.5 * n->n_dimp->fv_h;
		st.st_z += 0.5 * n->n_dimp->fv_w;

		st.st_lx = 1.0f;
		st.st_ly = 0.0f;
		st.st_lz = 0.0f;
		break;
	}
	tween_pop(TWF_LOOK | TWF_POS);
}

__inline int
node_show(struct node *n)
{
	if (n->n_flags & NF_HIDE || n->n_fillp->f_a == 0.0f)
		return (0);
	return (1);
}
