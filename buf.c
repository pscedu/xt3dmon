/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"

#define BUF_GROWAMT 30

__inline void
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
buf_append(struct buf *buf, int ch)
{
	if (++buf->buf_pos >= buf->buf_max) {
		buf->buf_max += BUF_GROWAMT;
		buf_realloc(buf);
	}
	buf->buf_buf[buf->buf_pos] = ch;
}

__inline void
buf_appendv(struct buf *buf, const char *s)
{
	while (*s != '\0')
		buf_append(buf, *s++);
}

void
buf_appendfv(struct buf *buf, const char *fmt, ...)
{
	char *s, *t;
	va_list ap;

	va_start(ap, fmt);
	if (vasprintf(&t, fmt, ap) == -1)
		err(1, "vasprintf");
	va_end(ap);

	for (s = t; *s != '\0'; s++)
		buf_append(buf, *s);
	free(t);
}

__inline char *
buf_get(const struct buf *buf)
{
	return (buf->buf_buf);
}

__inline void
buf_free(struct buf *buf)
{
	free(buf->buf_buf);
}

__inline void
buf_reset(struct buf *buf)
{
	buf->buf_pos = -1;
}

__inline void
buf_chop(struct buf *buf)
{
	buf->buf_pos--;
}

__inline int
buf_len(const struct buf *buf)
{
	return (buf->buf_pos + 1);
}
