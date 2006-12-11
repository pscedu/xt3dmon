/* $Id$ */

#include "objlist.h"
#include "fill.h"

#define YFL_CMD		100

struct yod {
	struct objhdr	 y_oh;
	int		 y_id;
	struct fill	 y_fill;

	int		 y_partid;
	char		 y_cmd[YFL_CMD];
	int		 y_ncpus;
	int		 y_single;
};

void		 yod_init(struct yod *);
struct yod	*yod_findbyid(int, int *);
int		 yod_cmp(const void *, const void *);
