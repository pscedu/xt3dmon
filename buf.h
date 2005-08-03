/* $Id$ */

#ifndef _BUF_H
#define _BUF_H

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_GROWAMT 30

struct buf {
	int	 buf_pos;
	int	 buf_max;
	int	 buf_opts; /* FATAL_ALLOCS */
	char	*buf_buf;
};

void	 buf_init(struct buf *);
void	 buf_realloc(struct buf *);
void	 buf_append(struct buf *, char);
char	*buf_get(struct buf *);
void	 buf_set(struct buf *, char *);
void	 buf_free(struct buf *);
void	 buf_reset(struct buf *);
void	 buf_chop(struct buf *);
int	 buf_len(struct buf *);
void	 buf_cat(struct buf *, struct buf *);

#endif /* _BUF_H */
