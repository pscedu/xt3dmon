/* $Id$ */

#include "mon.h"

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
node_physpos(struct node *node, int *r, int *cb, int *cg, int *m,
    int *n)
{
	int pos;

	pos = node - &nodes[0][0][0][0][0];

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
	int r, cb, cg, m, n, row, col;
	struct node *rn;
	struct ivec iv;
	int rem, adj;

	adj = 1;
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

	node_physpos(node, &r, &cb, &cg, &m, &n);
	iv = node->n_wiv;
	for (rem = abs(amt); rem > 0; rem--) {
		switch (st.st_vmode) {
		case VM_PHYSICAL:
			switch (dir) {
			case DIR_RIGHT:
				m += adj;
				if (m < 0) {
					cb--;
					m += NMODS;
					cb += NCABS;
				} else if (m >= NMODS)
					cb++;
				m %= NMODS;
				cb %= NCABS;
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
				if (n % 2)
					n++;
				else
					n--;
				if (n < 0)
					n += NNODES;
				n %= NNODES;
				node_getmodpos(n, &row, &col);
				if (adj == -1) {
					/*
					 * If we ended up in the top
					 * portion when traveling
					 * down, we wrapped.
					 */
					if (row == 1) {
						cg--;
						cg += NCAGES;
					}
				} else {
					/*
					 * If we ended up in the
					 * bottom portion, we wrapped.
					 */
					if (row == 0)
						cg++;
				}
				cg %= NCAGES;
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
				if (n % 2)
					n--;
				else
					n++;
				if (n < 0)
					n += NNODES;
				n %= NNODES;
				node_getmodpos(n, &row, &col);
				if (adj == -1) {
					/*
					 * If we ended up in the
					 * front column when moving
					 * back, we wrapped.
					 */
					if (col == 1) {
						r--;
						r += NROWS;
					}
				} else {
					/*
					 * If we ended up in the
					 * back column when moving
					 * forward, we wrapped.
					 */
					if (col == 0)
						r++;
				}
				r %= NROWS;
				break;
			}
			rn = &nodes[r][cb][cg][m][n];
			break;
		case VM_WIRED:
		case VM_WIREDONE:
			switch (dir) {
			case DIR_RIGHT:
				iv.iv_x += adj + WIDIM_WIDTH;
				iv.iv_x %= WIDIM_WIDTH;
				break;
			case DIR_UP:
				iv.iv_y += adj + WIDIM_HEIGHT;
				iv.iv_y %= WIDIM_HEIGHT;
				break;
			case DIR_FORWARD:
				iv.iv_z += adj + WIDIM_DEPTH;
				iv.iv_z %= WIDIM_DEPTH;
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
