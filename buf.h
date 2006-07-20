/* $Id$ */

#ifndef _BUF_H_
#define _BUF_H_

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
void	 buf_appendv(struct buf *, const char *);
void	 buf_appendfv(struct buf *, const char *, ...);
char	*buf_get(const struct buf *);
void	 buf_set(struct buf *, char *);
void	 buf_free(struct buf *);
void	 buf_reset(struct buf *);
void	 buf_chop(struct buf *);
int	 buf_len(const struct buf *);
void	 buf_cat(struct buf *, const struct buf *);

#endif /* _BUF_H_ */
