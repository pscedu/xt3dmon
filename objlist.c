/* $Id$ */

#include "mon.h"
#include "cdefs.h"

#define JINCR		10
#define YINCR		10
#define GINCR		10

int		 job_eq(const void *, const void *);
int		 yod_eq(const void *, const void *);
int		 glname_eq(const void *, const void *);

int		 job_cmp(const void *, const void *);
int		 yod_cmp(const void *, const void *);

struct objlist	 job_list    = { { NULL }, 0, 0, 0, 0, JINCR, sizeof(struct job),    0, job_eq, job_cmp };
struct objlist	 yod_list    = { { NULL }, 0, 0, 0, 0, YINCR, sizeof(struct yod),    0, yod_eq, yod_cmp };
struct objlist	 glname_list = { { NULL }, 0, 0, 0, 0, GINCR, sizeof(struct glname), 0, glname_eq, NULL };

struct nodeclass statusclass[] = {
	{ "Free",		FILL_INIT(1.00f, 1.00f, 1.00f) },
	{ "Disabled (PBS)",	FILL_INIT(1.00f, 0.00f, 0.00f) },
	{ "Down (CPA)",		FILL_INIT(0.66f, 0.66f, 0.66f) },
	{ NULL,			FILL_INIT(0.00f, 0.00f, 0.00f) },
	{ "Service",		FILL_INIT(1.00f, 1.00f, 0.00f) }
};

struct nodeclass tempclass[] = {
	{ "18-22C",		FILL_INIT(0.0f, 0.0f, 0.4f) },
	{ "22-26C",		FILL_INIT(0.8f, 0.0f, 0.4f) },
	{ "26-31C",		FILL_INIT(0.6f, 0.0f, 0.6f) },
	{ "31-35C",		FILL_INIT(0.4f, 0.0f, 0.8f) },
	{ "35-40C",		FILL_INIT(0.2f, 0.2f, 1.0f) },
	{ "40-44C",		FILL_INIT(0.0f, 0.0f, 1.0f) },
	{ "44-49C",		FILL_INIT(0.0f, 0.6f, 0.6f) },
	{ "49-53C",		FILL_INIT(0.0f, 0.8f, 0.0f) },
	{ "53-57C",		FILL_INIT(0.4f, 1.0f, 0.0f) },
	{ "57-62C",		FILL_INIT(1.0f, 1.0f, 0.0f) },
	{ "62-66C",		FILL_INIT(1.0f, 0.8f, 0.2f) },
	{ "66-71C",		FILL_INIT(1.0f, 0.6f, 0.0f) },
	{ "71-75C",		FILL_INIT(1.0f, 0.0f, 0.0f) },
	{ "75-80C",		FILL_INIT(1.0f, 0.6f, 0.6f) }
};

struct nodeclass failclass[] = {
	{ "0",			FILL_INIT(0.0f, 0.2f, 0.4f) },
	{ "1-5",		FILL_INIT(0.2f, 0.4f, 0.6f) },
	{ "6-10",		FILL_INIT(0.4f, 0.6f, 0.8f) },
	{ "11-15",		FILL_INIT(0.6f, 0.8f, 1.0f) },
	{ "15-20",		FILL_INIT(0.8f, 1.0f, 1.0f) },
	{ "20+",		FILL_INIT(1.0f, 1.0f, 1.0f) }
};

int
job_eq(const void *elem, const void *arg)
{
	return (((struct job *)elem)->j_id == *(int *)arg);
}

int
yod_eq(const void *elem, const void *arg)
{
	return (((struct yod *)elem)->y_id == *(int *)arg);
}

int
glname_eq(const void *elem, const void *arg)
{
	return (((struct glname *)elem)->gn_name == *(unsigned int *)arg);
}

int
job_cmp(const void *a, const void *b)
{
	return (CMP((*(struct job **)a)->j_id, (*(struct job **)b)->j_id));
}

int
yod_cmp(const void *a, const void *b)
{
	return (CMP((*(struct yod **)a)->y_id, (*(struct yod **)b)->y_id));
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
getobj(const void *arg, struct objlist *ol)
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

/*
 * For optimal colors, we would like to choose colors
 * in HSV mode where hue ranges from 0-360, saturation
 * ranges from 30%-100%, and value ranges from 50%-100%.
 */
void
getcol(int old, size_t n, size_t total, struct fill *fillp)
{
	float hinc, sinc, vinc;
	struct fill *scfp;

	/* Divide color wheel up evenly */
	hinc = 360.0 / (float)total;

	/* These could be random ... */
	sinc = (SAT_MAX - SAT_MIN) / (float)total;
	vinc = (VAL_MAX - VAL_MIN) / (float)total;

	fillp->f_h = hinc * n + HUE_MIN;
	fillp->f_s = sinc * n + SAT_MIN;
	fillp->f_v = vinc * n + VAL_MIN;
	if (!old) {
		fillp->f_a = 1.0;

		scfp = &statusclass[SC_USED].nc_fill;	/* XXX */
		fillp->f_texid[WINID_LEFT]    = scfp->f_texid[WINID_LEFT];
		fillp->f_texid[WINID_RIGHT]   = scfp->f_texid[WINID_RIGHT];
		fillp->f_texid_a[WINID_LEFT]  = scfp->f_texid_a[WINID_LEFT];
		fillp->f_texid_a[WINID_RIGHT] = scfp->f_texid_a[WINID_RIGHT];
	}

	hsv_to_rgb(fillp);
}
