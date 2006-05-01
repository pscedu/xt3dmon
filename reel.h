/* $Id$ */

struct reel {
	struct objhdr	rl_oh;
	char		rl_dirname[PATH_MAX];
	char		rl_name[40];
};

void reel_load(void);
int  reel_eq(const void *, const void *);
int  reel_cmp(const void *, const void *);

extern char reel_fn[PATH_MAX]; /* XXX */
