/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "fill.h"
#include "job.h"
#include "nodeclass.h"
#include "objlist.h"
#include "util.h"
#include "select.h"
#include "yod.h"

struct nodeclass statusclass[] = {
	{ "Free",		FILL_INIT(1.00f, 1.00f, 1.00f), 0 },
	{ "Disabled (PBS)",	FILL_INIT(1.00f, 0.00f, 0.00f), 0 },
	{ "Down (CPA)",		FILL_INIT(0.66f, 0.66f, 0.66f), 0 },
	{ "Allocated",		FILL_INITF(1.00f, 0.00f, 0.00f, FF_SKEL), 0 },
	{ "Service",		FILL_INIT(1.00f, 1.00f, 0.00f), 0 }
};

struct nodeclass tempclass[] = {
	{ "<22C",		FILL_INIT(0.0f, 0.0f, 0.4f), 0 },
	{ "22-25C",		FILL_INIT(0.0f, 0.0f, 0.8f), 0 },
	{ "26-29C",		FILL_INIT(0.0f, 0.0f, 1.0f), 0 },
	{ "30-33C",		FILL_INIT(0.0f, 0.6f, 1.0f), 0 },
	{ "34-37C",		FILL_INIT(0.0f, 0.8f, 1.0f), 0 },
	{ "38-41C",		FILL_INIT(0.0f, 1.0f, 1.0f), 0 },
	{ "42-45C",		FILL_INIT(0.8f, 1.0f, 1.0f), 0 },
	{ "46-49C",		FILL_INIT(1.0f, 1.0f, 1.0f), 0 },
	{ "50-53C",		FILL_INIT(1.0f, 1.0f, 0.6f), 0 },
	{ "54-57C",		FILL_INIT(1.0f, 1.0f, 0.0f), 0 },
	{ "58-61C",		FILL_INIT(1.0f, 0.8f, 0.0f), 0 },
	{ "62-65C",		FILL_INIT(1.0f, 0.6f, 0.0f), 0 },
	{ "66-69C",		FILL_INIT(1.0f, 0.4f, 0.0f), 0 },
	{ ">69C",		FILL_INIT(1.0f, 0.0f, 0.0f), 0 }
};

struct nodeclass failclass[] = {
	{ "0",			FILL_INIT(0.0f, 0.2f, 0.4f), 0 },
	{ "1-5",		FILL_INIT(0.2f, 0.4f, 0.6f), 0 },
	{ "6-10",		FILL_INIT(0.4f, 0.6f, 0.8f), 0 },
	{ "11-15",		FILL_INIT(0.6f, 0.8f, 1.0f), 0 },
	{ "15-20",		FILL_INIT(0.8f, 1.0f, 1.0f), 0 },
	{ ">20",		FILL_INIT(1.0f, 1.0f, 1.0f), 0 }
};

struct nodeclass rtclass[] = {
	{ "<10%",		FILL_INIT(1.0f, 1.0f, 0.4f), 0 },
	{ "10-20%",		FILL_INIT(1.0f, 0.9f, 0.4f), 0 },
	{ "20-30%",		FILL_INIT(1.0f, 0.8f, 0.4f), 0 },
	{ "30-40%",		FILL_INIT(1.0f, 0.7f, 0.4f), 0 },
	{ "40-50%",		FILL_INIT(1.0f, 0.6f, 0.4f), 0 },
	{ "50-60%",		FILL_INIT(1.0f, 0.5f, 0.4f), 0 },
	{ "60-70%",		FILL_INIT(1.0f, 0.4f, 0.4f), 0 },
	{ "70-80%",		FILL_INIT(1.0f, 0.3f, 0.4f), 0 },
	{ "80-90%",		FILL_INIT(1.0f, 0.2f, 0.4f), 0 },
	{ ">90%",		FILL_INIT(1.0f, 0.1f, 0.4f), 0 }
};
/*
 * rtpipeclass[] should be the same as rtclass[] above.
 * These are used to draw the pipes, and must be maintained
 * separately from rtclass[] so highlighting a class from
 * one group doesn't affect classes from the other group.
 */
struct nodeclass rtpipeclass[] = {
	{ "<10%",		FILL_INITA(1.0f, 1.0f, 0.4f, 0.5f), 0 },
	{ "10-20%",		FILL_INITA(1.0f, 0.9f, 0.4f, 0.5f), 0 },
	{ "20-30%",		FILL_INITA(1.0f, 0.8f, 0.4f, 0.5f), 0 },
	{ "30-40%",		FILL_INITA(1.0f, 0.7f, 0.4f, 0.5f), 0 },
	{ "40-50%",		FILL_INITA(1.0f, 0.6f, 0.4f, 0.5f), 0 },
	{ "50-60%",		FILL_INITA(1.0f, 0.5f, 0.4f, 0.6f), 0 },
	{ "60-70%",		FILL_INITA(1.0f, 0.4f, 0.4f, 0.7f), 0 },
	{ "70-80%",		FILL_INITA(1.0f, 0.3f, 0.4f, 0.8f), 0 },
	{ "80-90%",		FILL_INITA(1.0f, 0.2f, 0.4f, 0.9f), 0 },
	{ ">90%",		FILL_INITA(1.0f, 0.1f, 0.4f, 1.0f), 0 }
};

struct nodeclass ssclass[] = {
	{ "<10%",		FILL_INIT(1.0f, 0.0f, 0.0f), 0 },
	{ "10-20%",		FILL_INIT(1.0f, 0.3f, 0.0f), 0 },
	{ "20-30%",		FILL_INIT(1.0f, 0.6f, 0.0f), 0 },
	{ "30-40%",		FILL_INIT(1.0f, 0.9f, 0.0f), 0 },
	{ "40-50%",		FILL_INIT(1.0f, 1.0f, 0.0f), 0 },
	{ "50-60%",		FILL_INIT(0.8f, 1.0f, 0.0f), 0 },
	{ "60-70%",		FILL_INIT(0.6f, 1.0f, 0.0f), 0 },
	{ "70-80%",		FILL_INIT(0.4f, 1.0f, 0.0f), 0 },
	{ "80-90%",		FILL_INIT(0.2f, 1.0f, 0.0f), 0 },
	{ ">90%",		FILL_INIT(0.0f, 1.0f, 0.0f), 0 }
};

struct nodeclass lustreclass[] = {
	{ "Clean",		FILL_INIT(1.00f, 1.00f, 1.00f), 0 },
	{ "Dirty",		FILL_INIT(0.80f, 0.60f, 0.00f), 0 },
	{ "Recovering",		FILL_INIT(1.00f, 0.20f, 0.60f), 0 }
};

int
fe_eq(const void *elem, const void *arg)
{
	const struct fnent *fe = elem;

	return (strcmp(fe->fe_name, arg) == 0);
}

int
fe_cmp(const void *a, const void *b)
{
	const struct fnent **fea = (const struct fnent **)a;
	const struct fnent **feb = (const struct fnent **)b;

	return (strcmp((*fea)->fe_name, (*feb)->fe_name));
}

void
obj_batch_start(struct objlist *ol)
{
	ol->ol_tcur = 0;
}

void
obj_batch_end(struct objlist *ol)
{
	struct objhdr *ohp;
	size_t n;

	ol->ol_max = MAX(ol->ol_max, ol->ol_tcur);
	ol->ol_cur = ol->ol_tcur;
	if (ol->ol_data == NULL)		/* XXX */
		return;
	for (n = 0; n < ol->ol_cur; n++) {
		ohp = (struct objhdr *)ol->ol_data[n];
		ohp->oh_flags |= OHF_OLD;
	}
	for (; n < ol->ol_max; n++) {
		ohp = (struct objhdr *)ol->ol_data[n];
		ohp->oh_flags &= ~OHF_OLD;
	}
//	if (ol->ol_flags & OLF_SORT)
//		qsort(ol->ol_data, ol->ol_cur, sizeof(void *),
//		    ol->ol_cmpf);
}

/*
 * The "eq" routines must ensure that an object identifier can never be
 * zero; otherwise, they will match empty objects after memset().
 */
void *
obj_get(const void *arg, struct objlist *ol)
{
	struct objhdr *ohp;
	size_t n, max;
	void *j;

	if (ol->ol_data != NULL) {
		/* XXX: bsearch() */
		for (n = 0; n < ol->ol_tcur; n++)
			if (ol->ol_eq(ol->ol_data[n], arg))
				return (ol->ol_data[n]);
		for (; n < ol->ol_max; n++)
			if (ol->ol_eq(ol->ol_data[n], arg)) {
				j = ol->ol_data[ol->ol_tcur];
				ol->ol_data[ol->ol_tcur] = ol->ol_data[n];
				ol->ol_data[n] = j;
				goto new;
			}
	}
	/* Not found; add. */
	if (ol->ol_tcur >= ol->ol_alloc) {
		max = ol->ol_alloc + ol->ol_incr;
		if ((ol->ol_data = realloc(ol->ol_data,
		    max * sizeof(*ol->ol_data))) == NULL)
			err(1, "realloc");
		for (n = ol->ol_alloc; n < max; n++) {
			if ((j = malloc(ol->ol_objlen)) == NULL)
				err(1, "malloc");
			memset(j, 0, ol->ol_objlen);
			ol->ol_data[n] = j;
		}
		ol->ol_alloc = max;
	}
new:
//	memset(ol->ol_data[ol->ol_tcur], 0, ol->ol_objlen);
	ohp = ol->ol_data[ol->ol_tcur++];
	return (ohp);
}
