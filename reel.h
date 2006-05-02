/* $Id$ */

#include "objlist.h"

struct reel {
	struct objhdr	rl_oh;
	char		rl_dirname[PATH_MAX];
	char		rl_name[40];
};

struct reel_ent {
	struct objhdr	re_oh;
	char		re_name[NAME_MAX];
};

void reel_load(void);
int  reel_eq(const void *, const void *);
int  reel_cmp(const void *, const void *);
void reel_advance(void);
void reel_start(void);
void reel_end(void);

extern char		reel_fn[PATH_MAX]; /* XXX */
size_t			reel_pos;
extern struct objlist	reelent_list;

