/* $Id$ */

#include "route.h"
#include "xmath.h"

struct fill;
struct job;
struct yod;

#define NID_MAX		3000

struct node {
	int		 n_nid;
	struct fill	*n_fillp;
	int		 n_flags;
	int		 n_state;

	struct job	*n_job;
	struct yod	*n_yod;
	int		 n_temp;
	int		 n_fails;
	struct route	 n_route;

	struct ivec	 n_wiv;			/* wired view position */
	struct fvec	 n_swiv;		/* scaled */
	struct fvec	 n_physv;
	struct fvec	 n_vcur;
	struct fvec	*n_v;

	int		 n_dl[2];		/* display list ID */
};

#define NF_HIDE		(1<<0)
#define NF_SEL		(1<<1)
#define NF_DIRTY	(1<<3)			/* node needs recompiled */

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
			    (iv)->iv_z++)				\

struct node	*node_neighbor(struct node *, int, int);
void		 node_physpos(struct node *, struct physcoord *);
void		 node_getmodpos(int, int *, int *);
void		 node_adjmodpos(int, struct fvec *);
struct node	*node_for_nid(int);
void		 node_goto(struct node *);
int		 node_show(struct node *);

extern struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	*invmap[];
extern struct node	*wimap[WIDIM_WIDTH][WIDIM_HEIGHT][WIDIM_DEPTH];

extern int		 total_failures;	/* total among all nodes */
