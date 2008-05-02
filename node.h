/* $Id$ */

#include "route.h"
#include "seastar.h"
#include "xmath.h"

struct fill;
struct job;
struct yod;

struct node {
	int		 n_nid;
	struct fill	*n_fillp;
	int		 n_flags;
	int		 n_state;
	int		 n_lustat;
	int		 n_geom;

	struct job	*n_job;
	struct yod	*n_yod;
	int		 n_temp;
	struct route	 n_route;
	struct seastar	 n_sstar;

	struct ivec	 n_wiv;			/* wired view position */
	struct fvec	 n_vtwe;		/* where the node is going (tweening) */
	struct fvec	 n_v;			/* node position */
	struct fvec	*n_dimp;		/* dimensions */
};

#define NF_VALID	(1<<0)			/* valid node */
#define NF_SELNODE	(1<<1)			/* is selected */
#define NF_SUBSET	(1<<2)			/* subset membership */
#define NF_VMVIS	(1<<3)			/* visibility in vmode */

/* Lustre status. */
#define LS_CLEAN	0
#define LS_DIRTY	1
#define LS_RECOVER	2

#define NODE_FOREACH_PHYS(n, np)						\
	for (np = node_nidmap; np - node_nidmap < machine.m_nidmax; np++)	\
		if ((n = *np) != NULL)

#define NODE_FOREACH_WI(n, np)							\
	for (np = node_wimap; np - node_wimap < node_wimap_len; np++)		\
		if ((n = *np) != NULL)

struct node	*node_neighbor(int, struct node *, int, int *);
int		 node_wineighbor_nhops(const struct node *, const struct node *);
void		 node_physpos(struct node *, struct physcoord *);
void		 node_getmodpos(int, int *, int *);
void		 node_adjmodpos(int, struct fvec *);
void		 node_setphyspos(struct node *, struct fvec *);
struct node	*node_for_nid(int);
void		 node_goto(struct node *);
int		 node_show(const struct node *);
void		 node_center(const struct node *, struct fvec *);
struct node	*node_for_pc(const struct physcoord *);

#define NODE_WIMAP(x, y, z) \
	node_wimap[(x) * widim.iv_y * widim.iv_z + (y) * widim.iv_z + (z)]

extern struct node	 *nodes;
extern struct node	**node_nidmap;
extern struct node	**node_wimap;
extern int		  node_wimap_len;
