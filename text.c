/* $Id$ */

#include <sys/types.h>

#include <ctype.h>
#include <string.h>

void
text_wrap(char *s, size_t len, size_t maxlinelen)
{
	size_t linelen;
	char *p, *t;

	linelen = 0;
	for (p = s; *p != '\0'; p++) {
		if (++linelen > maxlinelen) {
			for (t = p; t > s && *t != '\n'; t--)
				if (!isalnum(*t))
					break;
			if (t == s || *t++ == '\0')
				t = p;
			memmove(t + 1, t, len - 1 - (t - s));
			s[len - 1] = '\0';
			*t = '\n';
			linelen = p - t;
		}
	}
}
