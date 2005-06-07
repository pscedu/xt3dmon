/* $Id$ */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#define BUF_GROWAMT 30

struct buf {
	int	 buf_pos;
	int	 buf_max;
	int	 buf_opts; /* FATAL_ALLOCS */
	char	*buf_buf;
};

void	 buf_init(struct buf *buf);
void	 buf_realloc(struct buf *buf);
void	 buf_append(struct buf *buf, char ch);
char	*buf_get(struct buf *buf);
void	 buf_set(struct buf *buf, char *s);
void	 buf_free(struct buf *buf);
void	 buf_reset(struct buf *buf);
void	 buf_chop(struct buf *buf);
int	 buf_len(struct buf *buf);
void	 buf_cat(struct buf *ba, struct buf *bb);
