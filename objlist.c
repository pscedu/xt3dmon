/* $Id$ */

#include "cdefs.h"
#include "mon.h"

#define JINCR		10
#define FINCR		10
#define TINCR		10
#define GINCR		10

int		 job_eq(const void *, const void *);
int		 fail_eq(const void *, const void *);
int		 temp_eq(const void *, const void *);
int		 glname_eq(const void *, const void *);

struct objlist	 job_list  = { { NULL }, 0, 0, 0, 0, JINCR, sizeof(struct job), job_eq };
struct objlist	 temp_list = { { NULL }, 0, 0, 0, 0, TINCR, sizeof(struct temp), fail_eq };
struct objlist	 fail_list = { { NULL }, 0, 0, 0, 0, FINCR, sizeof(struct fail), temp_eq };

struct objlist	 glname_list = { { NULL }, 0, 0, 0, 0, GINCR, sizeof(struct glname), glname_eq };

struct fail fail_notfound = {
	{ 0 }, 0, { 0.33f, 0.66f, 0.99f, 1.00f, 0, 0 }, "0"
};

struct temp temp_notfound = {
	{ 0 }, 0, { 0.00f, 0.00f, 0.00f, 1.00f, 0, 0 }, "?"
};

struct temp_range temp_map[] = {
	{ { 0.0f, 0.0f, 0.4f, 1.0f, 0, 0 }, "18-22C" },
	{ { 0.8f, 0.0f, 0.4f, 1.0f, 0, 0 }, "22-26C" },
	{ { 0.6f, 0.0f, 0.6f, 1.0f, 0, 0 }, "26-31C" },
	{ { 0.4f, 0.0f, 0.8f, 1.0f, 0, 0 }, "31-35C" },
	{ { 0.2f, 0.2f, 1.0f, 1.0f, 0, 0 }, "35-40C" },
	{ { 0.0f, 0.0f, 1.0f, 1.0f, 0, 0 }, "40-44C" },
	{ { 0.0f, 0.6f, 0.6f, 1.0f, 0, 0 }, "44-49C" },
	{ { 0.0f, 0.8f, 0.0f, 1.0f, 0, 0 }, "49-53C" },
	{ { 0.4f, 1.0f, 0.0f, 1.0f, 0, 0 }, "53-57C" },
	{ { 1.0f, 1.0f, 0.0f, 1.0f, 0, 0 }, "57-62C" },
	{ { 1.0f, 0.8f, 0.2f, 1.0f, 0, 0 }, "62-66C" },
	{ { 1.0f, 0.6f, 0.0f, 1.0f, 0, 0 }, "66-71C" },
	{ { 1.0f, 0.0f, 0.0f, 1.0f, 0, 0 }, "71-75C" },
	{ { 1.0f, 0.6f, 0.6f, 1.0f, 0, 0 }, "75-80C" }
};

int
job_eq(const void *elem, const void *arg)
{
	return (((struct job *)elem)->j_id == *(int *)arg);
}

int
temp_eq(const void *elem, const void *arg)
{
	return (((struct temp *)elem)->t_cel == *(int *)arg);
}

int
fail_eq(const void *elem, const void *arg)
{
	return (((struct fail *)elem)->f_fails == *(int *)arg);
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
temp_cmp(const void *a, const void *b)
{
	return (CMP((*(struct temp **)a)->t_cel, (*(struct temp **)b)->t_cel));
}

int
fail_cmp(const void *a, const void *b)
{
	return (CMP((*(struct fail **)a)->f_fails, (*(struct fail **)b)->f_fails));
}

void
obj_batch_start(struct objlist *ol)
{
	size_t n;

	ol->ol_tcur = 0;
	if (ol->ol_data == NULL)
		return;
	for (n = 0; n < ol->ol_cur; n++)
		((struct objhdr *)ol->ol_data[n])->oh_flags &= ~OHF_TREF;
}

void
obj_batch_end(struct objlist *ol)
{
	struct objhdr *ohp, *swapohp;
	size_t n, lookpos;
	void *t;

	ol->ol_max = MAX(ol->ol_max, ol->ol_tcur);
	ol->ol_cur = ol->ol_tcur;
	if (ol->ol_data == NULL)
		return;
	lookpos = 0;
	for (n = 0; n < ol->ol_cur; n++) {
		ohp = (struct objhdr *)ol->ol_data[n];
		if (ohp->oh_flags & OHF_TREF)
			ohp->oh_flags |= OHF_REF;
		else {
			if (lookpos <= n)
				lookpos = n + 1;
			/* Scan forward to swap. */
			for (; lookpos < ol->ol_max; lookpos++) {
				swapohp = ol->ol_data[lookpos];
				if (swapohp->oh_flags & OHF_TREF) {
					t = ol->ol_data[n];
					ol->ol_data[n] = ol->ol_data[lookpos];
					ol->ol_data[lookpos++] = t;
					break;
				}
			}
			if (lookpos == ol->ol_max) {
				ol->ol_cur = n + 1;
				return;
			}
		}
	}
}

/*
 * The "eq" routines must ensure that an object identifier can never be
 * zero; otherwise, they will match empty objects after memset().
 */
void *
getobj(const void *arg, struct objlist *ol)
{
	void **jj, *j = NULL;
	struct objhdr *ohp;
	size_t n, max;

	if (ol->ol_data != NULL) {
		max = MAX(ol->ol_max, ol->ol_tcur);
		for (n = 0, jj = ol->ol_data; n < max; jj++, n++) {
			ohp = (struct objhdr *)*jj;
			if (ol->ol_eq(ohp, arg))
				goto found;
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
//	memset(ol->ol_data[ol->ol_tcur], 0, ol->ol_objlen);
	ohp = ol->ol_data[ol->ol_tcur];
found:
	if ((ohp->oh_flags & OHF_TREF) == 0) {
		ol->ol_tcur++;
		ohp->oh_flags |= OHF_TREF;
	}
	return (ohp);
}

/*
 * For optimal colors, we would like to choose colors
 * in HSV mode where hue ranges from 0-360, saturation
 * ranges from 30%-100%, and value ranges from 50%-100%.
 */
void
getcol(size_t n, size_t total, struct fill *fillp)
{
	float hinc, sinc, vinc;

	/* Divide color wheel up evenly */
	hinc = 360.0 / (float)(total);

	/* These could be random ... */
	sinc = (SAT_MAX - SAT_MIN) / (float)(total);
	vinc = (VAL_MAX - VAL_MIN) / (float)(total);

	fillp->f_h = hinc * n + HUE_MIN;
	fillp->f_s = sinc * n + SAT_MIN;
	fillp->f_v = vinc * n + VAL_MIN;
	fillp->f_a = 1.0;

	hsv_to_rgb(fillp);
}

void
getcol_temp(size_t n, struct fill *fillp)
{
	int cel = temp_list.ol_temps[n]->t_cel;
	int idx;

	if (cel < TEMP_MIN)
		cel = TEMP_MIN;
	else if (cel > TEMP_MAX)
		cel = TEMP_MAX;

	idx = (cel - TEMP_MIN) * TEMP_NTEMPS / (TEMP_MAX - TEMP_MIN);

	*fillp = temp_map[idx].m_fill;
}
