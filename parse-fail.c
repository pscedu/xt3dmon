/* $Id$ */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "mon.h"

#define MAXFAILS INT_MAX

/*
 * Parse failure data entries.
 *
 * Format:
 *	1		2724
 *	<nfailures>	<nid>
 *
 * Notes:
 *	zero data is not listed.
 */
void
parse_failmap(void)
{
	int nid, lineno, fails, r, cb, cg, m, n;
	char *p, *s, fn[PATH_MAX], buf[BUFSIZ];
	FILE *fp;
	size_t j;
	long l;

	/*
	 * Because entries with zero failures are not listed,
	 * we must go through and reset all entries.
	 */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++)
						nodes[r][cb][cg][m][n].n_fails = 0;

	for (j = 0; j < NLOGIDS; j++) {
		snprintf(fn, sizeof(fn), _PATH_FAILMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL) {
			warn("%s", fn);
			continue;
		}
		lineno = 0;
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			lineno++;
			p = buf;
			while (isspace(*p))
				p++;
			for (s = p; isdigit(*s); s++)
				;
			if (s == p || !isspace(*s))
				goto bad;
			*s = '\0';
			if ((l = strtol(p, NULL, 10)) < 0 || l >= MAXFAILS)
				goto bad;
			fails = (int)l;

			p = s + 1;
			while (isspace(*p))
				p++;
			for (s = p; isdigit(*s); s++)
				;
			if (s == p || *s != '\n')
				goto bad;
			*s = '\0';
			if ((l = strtol(p, NULL, 10)) < 0 || l >= NID_MAX)
				goto bad;
			nid = (int)l;

			invmap[j][nid]->n_fails = fails;
			continue;
bad:
			warnx("%s:%d: malformed line", fn, lineno);
		}
		if (ferror(fp))
			warn("%s", fn);
		fclose(fp);
		errno = 0;
	}

}
