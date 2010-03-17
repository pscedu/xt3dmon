/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

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
void	 buf_append(struct buf *, int);
void	 buf_appendv(struct buf *, const char *);
void	 buf_appendfv(struct buf *, const char *, ...);
char	*buf_get(const struct buf *);
void	 buf_free(struct buf *);
void	 buf_reset(struct buf *);
void	 buf_chop(struct buf *);
int	 buf_len(const struct buf *);

#endif /* _BUF_H_ */
