/* $Id$ */

#include "mon.h"

struct yod *
yod_findbyid(int id)
{
	size_t n;

	for (n = 0; n < yod_list.ol_cur; n++)
		if (yod_list.ol_yods[n]->y_id == id)
			return (yod_list.ol_yods[n]);
	return (NULL);
}
