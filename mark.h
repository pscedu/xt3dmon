/* $Id$ */

#include "queue.h"
#include "xmath.h"

struct mark {
	struct fvec		m_fv;
	SLIST_ENTRY(mark)	m_link;
};

SLIST_HEAD(, mark) marks;
