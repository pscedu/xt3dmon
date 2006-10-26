/* $Id$ */

#include "queue.h"
#include "xmath.h"

struct selnode {
	struct node		 *sn_nodep;
	struct fvec		  sn_offv;
	SLIST_ENTRY(selnode)	  sn_next;
};

SLIST_HEAD(selnodes, selnode);

void	sn_add(struct node *, const struct fvec *);
void	sn_clear(void);
int	sn_del(struct node *);
void	sn_replace(struct selnode *, struct node *);
void	sn_set(struct node *, const struct fvec *);
void	sn_toggle(struct node *, const struct fvec *);
void	sn_addallvis(void);

extern struct selnodes	selnodes;
extern size_t		nselnodes;
