/* $Id$ */

#include "queue.h"

struct selnode {
	struct node		 *sn_nodep;
	SLIST_ENTRY(selnode)	  sn_next;
};

SLIST_HEAD(selnodes, selnode);

void	sn_add(struct node *);
void	sn_clear(void);
int	sn_del(struct node *);
void	sn_insert(struct node *);
void	sn_replace(struct selnode *, struct node *);
void	sn_set(struct node *);
void	sn_toggle(struct node *);

extern struct selnodes	selnodes;
extern size_t		nselnodes;
