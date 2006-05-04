/* $Id$ */

#include "queue.h"
#include "xmath.h"

struct mark {
	struct fvec		m_fv;
	SLIST_ENTRY(mark)	m_link;
};

SLIST_HEAD(markhd, mark);

void mark_add(struct fvec *);
void mark_clear(void);
void mark_draw(void);

extern struct markhd marks;
