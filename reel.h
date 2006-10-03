/* $Id$ */

#include "objlist.h"

#define LATEST_REEL	"data/latest-archive"

char *reel_set(const char *, int);
void  reel_load(void);
void  reel_advance(void);
void  reel_start(void);
void  reel_end(void);

extern char	reel_dir[];
extern char	reel_browsedir[];
size_t		reel_pos;
