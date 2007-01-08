/* $Id$ */

/*
 * Routines for managing yods.
 * "Yods" are system allocations known by the
 * scheduler for running jobs on the XT3.
 */

#include "mon.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "objlist.h"
#include "util.h"
#include "yod.h"

#define YINCR	10

int yod_eq(const void *, const void *);

struct objlist yod_list = { NULL, 0, 0, 0, 0, YINCR, sizeof(struct yod), yod_eq };

int
yod_cmp(const void *a, const void *b)
{
	return (CMP((*(struct yod **)a)->y_id, (*(struct yod **)b)->y_id));
}

int
yod_eq(const void *elem, const void *arg)
{
	return (((struct yod *)elem)->y_id == *(int *)arg);
}

struct yod *
yod_findbyid(int id, int *pos)
{
	size_t n;

	for (n = 0; n < yod_list.ol_cur; n++)
		if (OLE(yod_list, n, yod)->y_id == id) {
			if (pos)
				*pos = n;
			return (OLE(yod_list, n, yod));
		}
	if (pos)
		*pos = -1;
	return (NULL);
}

/*
 * Initialize a yod structure.
 * - Determine if the allocation uses single or dual cores
 *   for each of the nodes in the yod.  To do this, we must
 *   parse the yod command and see if "-SN" was specified.
 */
void
yod_init(struct yod *y)
{
	const char *p, *t;
	int sawflag;

	y->y_single = 0;			/* defaults to single */
	sawflag = 0;				/* `-flag' seen */
	t = y->y_cmd;
	while (isspace(*t))			/* skip prepended space */
		t++;
	for (p = t; *p != '\0'; p++)
		if (isalnum(*p) || *p == '.' || *p == '/') {
			/*
			 * If this isn't the first word, then it
			 * is an argument, not an option flag,
			 * hence the rest of the command line
			 * should not be parsed.
			 */
			if (p > t && !sawflag)
				break;
			sawflag = 0;

			/* Skip word. */
			while (isalpha(p[1]) || p[1] == ',' ||
			    p[1] == '.')
				p++;
		} else if (*p == '-') {
			if (strncmp(p + 1, "SN", 2) == 0 &&
			    (isspace(p[3]) || p[3] == '\0')) {
				y->y_single = 1;
				break;
			}
			sawflag = 1;

			/* Skip word. */
			while (isalnum(p[1]) || p[1] == '=' ||
			    p[1] == '_')
				p++;
		}
}
