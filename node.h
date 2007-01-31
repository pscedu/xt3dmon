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
#define NF_SUBSEL	(1<<2)			/* subselection membership */
#define NF_VMVIS	(1<<3)			/* visibility in vmode */

/* Lustre status. */
#define LS_CLEAN	0
#define LS_DIRTY	1
#define LS_RECOVER	2

#define NODE_FOREACH(n, iv)						\
	for ((iv)->iv_x = 0;						\
	    (iv)->iv_x < widim.iv_w;					\
	    (iv)->iv_x++)						\
		for ((iv)->iv_y = 0;					\
		    (iv)->iv_y < widim.iv_h;				\
		    (iv)->iv_y++)					\
			for ((iv)->iv_z = 0;				\
			    (iv)->iv_z < widim.iv_d &&			\
			    (((n) = wimap[(iv)->iv_x][(iv)->iv_y]	\
			        [(iv)->iv_z]) || 1);			\
			    (iv)->iv_z++)
//				if (n)

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

extern struct node	  nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	**invmap;
extern struct node	 *wimap[WIDIM_WIDTH][WIDIM_HEIGHT][WIDIM_DEPTH];
