/* $Id$ */

#include <err.h>
#include <limits.h>
#include <stdio.h>

#include "mon.h"

#define ST_COORD	0
#define ST_CPU		1
#define ST_NID		2
#define ST_TYPE		3

/*
 * Format:
 *  0        1   2   3
 *  coord    cpu nid type (cni)
 *  ===========================
 *  c1-0c0s0 0   0   i
 */
void
parse_physmap(void)
{
	char sav, *s, *p, fn[MAXPATHLEN], buf[BUFSIZ];
	int r, cb, cg, m, n, nid, j, st, ln;
	unsigned long l;
	FILE *fp;

	for (j = 0; j < NLOGIDS; j++) {
		snprintf(fn, sizeof(fn), _PATH_PHYSMAP, j);
		if ((fp = fopen(fn, "r")) == NULL)
			err(1, "%s", fn);
		ln = st = 0;
		for (ln = 1; fgets(buf, sizeof(buf), fp) != NULL, ln++) {
			for (s = buf; *s != '\0'; s++) {
				while (isspace(*s))
					s++;
				if (*s == '#')
					continue;
				switch (st) {
				case ST_COORD:
					break;
				case ST_CPU:
					for (p = s; isdigit(*p); p++)
						;
					if (p == s)
						goto badline;
					break;
				case ST_NID:
					for (p = s; isdigit(*p); p++)
						;
					if (p == s)
						goto badline;
					sav = *++p;
					*p = '\0';
					if ((l = strtoul(s, NULL, 10)) > INT_MAX) {
						warn("[%s:%d] invalid nid %s",
						    fn, ln, s);
						goto nextline;
					}
					nid = (int)l;
					*p = sav;
					break;
				case ST_TYPE:
					switch (*s) {
					case 'c':
						type = T_COMPUTE;
						break;
					case 'n':
						type = T_DISABLE;
						break;
					case 'i':
						type = T_IO;
						break;
					default:
						goto badline;
					}
					break;
				default:
badline:
					warn("[%s:%d] malformed line: %s",
					    fn, ln, buf);
					goto nextline;
				}
			}
nextline:
			;
		}
		fclose(fp);
	}
}

