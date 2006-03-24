/* $Id$ */

#include "mon.h"

#include <stdio.h>

#include "yod.h"
#include "objlist.h"

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
