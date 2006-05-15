/* $Id$ */

#include "queue.h"
#include "xmath.h"

struct lnseg {
	struct fvec		ln_sv;
	struct fvec		ln_ev;
	SLIST_ENTRY(lnseg)	ln_link;
};

SLIST_HEAD(lnseghd, lnseg);

void lnseg_add(struct fvec *, struct fvec *);
void lnseg_clear(void);
void lnseg_draw(void);

extern struct lnseghd lnsegs;
