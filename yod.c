/* $Id$ */

#include "mon.h"

#include <stdio.h>

#include "objlist.h"
#include "util.h"
#include "yod.h"

#define YINCR	10

int yod_eq(const void *, const void *);

struct objlist yod_list = { { NULL }, 0, 0, 0, 0, YINCR, sizeof(struct yod), yod_eq };

int
yod_cmp(const void *a, const void *b)
{
	return (CMP((*(struct yod **)a)->y_id, (*(struct yod **)b)->y_id));
}

int
yod_eq(const void *elem, const void *arg)
{
	return (((struct yod *)elem)->y_id == *(int *)arg);
}

struct yod *
yod_findbyid(int id, int *pos)
{
	size_t n;

	for (n = 0; n < yod_list.ol_cur; n++)
		if (yod_list.ol_yods[n]->y_id == id) {
			if (pos)
				*pos = n;
			return (yod_list.ol_yods[n]);
		}
	if (pos)
		*pos = -1;
	return (NULL);
}
