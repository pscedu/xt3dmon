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

#include "mon.h"

#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "buf.h"
#include "ustream.h"
#include "util.h"
#include "cdefs.h"

/* Special case of base 2 to base 10. */
int
baseconv(int n)
{
	unsigned int i;

	for (i = 0; i < sizeof(n) * 8; i++)
		if (n & (1 << i))
			return (i + 1);
	return (0);
}

/*
 * enc buffer must be 4/3+1 the size of buf.
 * Note: enc and buf are NOT C-strings.
 */
void
base64_encode(const void *buf, char *enc, size_t siz)
{
	static char pres[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz0123456789+/";
	const unsigned char *p;
	u_int32_t val;
	size_t pos;
	int i;

	i = 0;
	for (pos = 0, p = buf; pos < siz; pos += 3, p += 3) {
		/*
		 * Convert 3 bytes of input (3*8 bits) into
		 * 4 bytes of output (4*6 bits).
		 *
		 * If fewer than 3 bytes are available for this
		 * round, use zeroes in their place.
		 */
		val = p[0] << 16;
		if (pos + 1 < siz)
			val |= p[1] << 8;
		if (pos + 2 < siz)
			val |= p[2];

		enc[i++] = pres[val >> 18];
		enc[i++] = pres[(val >> 12) & 0x3f];
		if (pos + 1 >= siz)
			break;
		enc[i++] = pres[(val >> 6) & 0x3f];
		if (pos + 2 >= siz)
			break;
		enc[i++] = pres[val & 0x3f];
	}
	if (pos + 1 >= siz) {
		enc[i++] = '=';
		enc[i++] = '=';
	} else if (pos + 2 >= siz)
		enc[i++] = '=';
	enc[i++] = '\0';
printf("base64: wrote %d chars\n", i);
}

/* Like strchr, but bound before NUL. */
char *
strnchr(const char *s, char c, size_t len)
{
	size_t pos;

	for (pos = 0; s[pos] != c && pos < len; pos++)
		;

	if (pos == len)
		return (NULL);
	return ((char *)(s + pos));
}

const char *
smart_basename(const char *fn)
{
	static char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s", fn);
	return (basename(path));
}

const char *
smart_dirname(const char *fn)
{
	static char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s", fn);
	return (dirname(path));
}

void
escape_printf(struct buf *bufp, const char *s)
{
	for (; *s != '\0'; s++)
		switch (*s) {
		case '%':
			buf_append(bufp, '%');
			/* FALLTHROUGH */
		default:
			buf_append(bufp, *s);
			break;
		}
}

void
fmt_scaled(size_t bytes, char *buf)
{
	const char tab[] = "KMGTE";
	int lvl;
	double siz;

	lvl = -1;
	siz = bytes;
	do {
		lvl++;
		siz /= 1024.0;
	} while (round(siz * 100.0) >= 100000.0 && lvl < (int)strlen(tab));
	snprintf(buf, FMT_SCALED_BUFSIZ, "%.2f%cB", siz, tab[lvl]);
}

char *
my_fgets(struct ustream *usp, char *s, int siz,
    ssize_t (*readf)(struct ustream *, size_t))
{
	size_t total, chunksiz;
	char *ret, *nl, *endp;
	int remaining;
	ssize_t nr;

	remaining = siz - 1;		/* NUL termination. */
	total = 0;
	ret = s;
	while (remaining > 0) {
		/* Look for newline in current buffer. */
		if (usp->us_bufstart) {
			if ((nl = strnchr(usp->us_bufstart, '\n',
			    usp->us_bufend - usp->us_bufstart + 1)) != NULL)
				endp = nl;
			else
				endp = usp->us_bufend;
			chunksiz = MIN(endp - usp->us_bufstart + 1, remaining);

			/* Copy all data up to any newline. */
			memcpy(s + total, usp->us_bufstart, chunksiz);
			remaining -= chunksiz;
			total += chunksiz;
			usp->us_bufstart += chunksiz;
			if (usp->us_bufstart > usp->us_bufend)
				usp->us_bufstart = NULL;
			if (nl)
				break;
		}

		/* Not found, read more. */
		chunksiz = MIN(remaining, (int)sizeof(usp->us_buf));
		nr = readf(usp, chunksiz);
		usp->us_lastread = nr;

		if (nr == -1 || nr == 0)
			return (NULL);

		usp->us_bufstart = usp->us_buf;
		usp->us_bufend = usp->us_buf + nr - 1;
	}
	/*
	 * This should be safe because total is
	 * bound by siz - 1.
	 */
	s[total] = '\0';
	return (ret);
}
