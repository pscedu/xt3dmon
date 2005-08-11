/* $Id$ */

#include "mon.h"

void
node_physpos(struct node *node, int *r, int *cb, int *cg, int *m,
    int *n)
{
	int pos;

	pos = (node - &nodes[0][0][0][0][0]) / sizeof(*node);

	*r = pos / (NCABS * NCAGES * NMODS * NNODES);
	pos %= NCABS * NCAGES * NMODS * NNODES;
	*cb = pos / (NCAGES * NMODS * NNODES);
	pos %= NCAGES * NMODS * NNODES;
	*cg = pos / (NMODS * NNODES);
	pos %= NMODS * NNODES;
	*m = pos / NNODES;
	pos %= NNODES;
	*n = pos;
}

struct node *
node_neighbor(struct node *node, int amt, int dir)
{
	int r, cb, cg, m, n;
	struct node *rn;
	struct ivec iv;

	if (dir == DIR_LEFT ||
	    dir == DIR_DOWN ||
	    dir == DIR_BACK)
		amt *= -1;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		node_physpos(node, &r, &cb, &cg, &m, &n);
		switch (dir) {
		case DIR_RIGHT:
			m += amt;
			cb += m / NMODS;
			m %= NMODS;
			cb %= NCABS;
			break;
		case DIR_UP:
			if (amt % 2) {
				/*
				 * Moving an odd number of nodes,
				 * which means we move from our
				 * position within a blade.
				 *
				 *	n | need
				 *	=========
				 *	0 | 3
				 *	3 | 0
				 *	2 | 1
				 *	1 | 2
				 */
				if (n % 2)
					n--;
				else
					n++;
				n %= NNODES;
			}
			cg += amt / 2;
			cg %= NCAGES;
			break;
		case DIR_FORWARD:
			if (amt % 2) {
				/*
				 * Moving an odd number of nodes,
				 * which means we move from our
				 * position within a blade.
				 *
				 *	n | need
				 *	=========
				 *	0 | 1
				 *	1 | 0
				 *	2 | 3
				 *	3 | 2
				 */
				if (n % 2)
					n--;
				else
					n++;
			}
			r += amt / 2;
			r %= NROWS;
			break;
		}
		rn = &nodes[r][cb][cg][m][n];
		break;
	case VM_WIRED:
	case VM_WIREDONE:
		iv = node->n_wiv;
		switch (dir) {
		case DIR_RIGHT:
			iv.iv_x += amt;
			iv.iv_x %= WIDIM_WIDTH;
			break;
		case DIR_UP:
			iv.iv_y += amt;
			iv.iv_y %= WIDIM_HEIGHT;
			break;
		case DIR_FORWARD:
			iv.iv_z += amt;
			iv.iv_z %= WIDIM_DEPTH;
			break;
		}
		rn = wimap[iv.iv_x][iv.iv_y][iv.iv_z];
		break;
	}
	return (rn);
}
