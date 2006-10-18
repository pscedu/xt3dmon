/* $Id$ */

#include "mon.h"

#include <ctype.h>
#include <string.h>

#include "cdefs.h"
#include "buf.h"

void
text_wrap(struct buf *bufp, const char *s, size_t maxlen, const char *ins,
    size_t mocklen)
{
	const char *p, *t, *brkp, *sp;
	size_t linelen, inslen;
	long i;

	inslen = strlen(ins);

	buf_init(bufp);

	linelen = 0;
	for (p = s; *p != '\0'; p++) {
		if (*p == '\n') {
			linelen = 0;
			buf_append(bufp, '\n');
			while (isspace(*++p))
				;
			p--;
		} else if (++linelen > maxlen) {
			/* First attempt: try to break on whitespace. */
			brkp = NULL;
			for (t = p - 1; p - t < (long)(linelen - mocklen) && *t != '\n'; t--) {
				if (!isalnum(*t) && brkp == NULL)
					brkp = t + 1;
				if (isspace(*t)) {
					for (sp = t - 1; sp > s && isspace(*sp); sp--)
						;
					sp++;
					for (i = 0; i < p - sp; i++)
						buf_chop(bufp);
					buf_appendv(bufp, ins);
					linelen = mocklen;
					p = t;
					while (isspace(*++p))
						;
					p--;
					goto next;
				}
			}
			/*
			 * If no suitable break position was found,
			 * force break at current end.
			 */
			if (brkp == NULL)
				t = p;
			else
				t = brkp;

			for (i = 0; i < p - t; i++)
				buf_chop(bufp);
			buf_appendv(bufp, ins);
			linelen = mocklen;
			p = t - 1;
			while (isspace(*p))
				p++;
		} else
			buf_append(bufp, *p);
next:
		;
	}
	buf_append(bufp, '\0');
}
