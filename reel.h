/* $Id$ */

#include "objlist.h"

void reel_load(void);
void reel_advance(void);
void reel_start(void);
void reel_end(void);

extern char		reel_fn[PATH_MAX]; /* XXX */
size_t			reel_pos;
extern struct objlist	reelent_list;
