/* $Id$ */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"

#define BUF_GROWAMT 30

void
buf_init(struct buf *buf)
{
	buf->buf_pos = buf->buf_max = -1;
	buf->buf_buf = NULL;
}

void
buf_realloc(struct buf *buf)
{
	void *ptr;

	if ((ptr = realloc(buf->buf_buf,
	     buf->buf_max * sizeof(*buf->buf_buf))) == NULL)
		err(1, "realloc");
	buf->buf_buf = ptr;
}

void
buf_append(struct buf *buf, char ch)
{
	if (++buf->buf_pos >= buf->buf_max) {
		buf->buf_max += BUF_GROWAMT;
		buf_realloc(buf);
	}
	buf->buf_buf[buf->buf_pos] = ch;
}

char *
buf_get(struct buf *buf)
{
	return (buf->buf_buf);
}

void
buf_set(struct buf *buf, char *s)
{
	/* XXX:  adjust pos and max. */
	buf->buf_buf = s;
}

void
buf_free(struct buf *buf)
{
	free(buf->buf_buf);
}

void
buf_reset(struct buf *buf)
{
	buf->buf_pos = -1;
}

void
buf_chop(struct buf *buf)
{
	buf->buf_pos--;
}

int
buf_len(struct buf *buf)
{
	return (buf->buf_pos + 1);
}

void
buf_cat(struct buf *ba, struct buf *bb)
{
	if (ba->buf_max - ba->buf_pos <= bb->buf_pos) {
		ba->buf_max = ba->buf_pos + bb->buf_pos + BUF_GROWAMT;
		buf_realloc(ba);
	}
	strncpy(ba->buf_buf + ba->buf_pos + 1, bb->buf_buf,
	    bb->buf_pos + 1);
	ba->buf_pos += bb->buf_pos;
}
