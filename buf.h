/* $Id$ */

/*
 * Simple routines for manipulating variable-sized
 * buffers.
 *
 * TODO: add the following members:
 *	- buf_opts with option BUFOPT_FATAL_ALLOC
 *	- buf_growamt to control growth size
 */

#ifndef _BUF_H_
#define _BUF_H_

#define BUF_GROWAMT 30

struct buf {
	int	 buf_pos;
	int	 buf_max;
	char	*buf_buf;
};

#define buf_nul(buf) buf_append((buf), '\0')

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

#endif /* _BUF_H_ */
