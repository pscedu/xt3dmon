/* $Id$ */

#include "mon.h"

#include <ctype.h>
#include <string.h>

void
text_wrap(char *s, size_t len, size_t maxlinelen)
{
	char *p, *t, *v;
	size_t linelen;

	linelen = 0;
	for (p = s; *p != '\0'; p++) {
		if (*p == '\n')
			linelen = 0;
		else if (++linelen > maxlinelen) {
			/* First attempt: try to break on whitespace. */
			v = NULL;
			for (t = p; t > s && *t != '\n'; t--) {
				if (!isalnum(*t) && v == NULL)
					v = t;
				if (isspace(*t))
					goto insertnl;
			}
			t = v;
			if (t == s || *t++ == '\0')
				t = p;
			memmove(t + 1, t, len - 1 - (t - s));
			s[len - 1] = '\0';
insertnl:
			*t = '\n';
			linelen = p - t;
		}
	}
}
