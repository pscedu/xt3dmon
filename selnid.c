/* $Id$ */

#include "mon.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "pathnames.h"
#include "node.h"
#include "selnode.h"
#include "status.h"

void
selnid_load(void)
{
	char buf[BUFSIZ];
	struct node *n;
	const char *fn;
	FILE *fp;
	int nid;

	fn = _PATH_SELNIDS;
	if ((fp = fopen(fn, "r")) == NULL) {
		status_add(SLP_URGENT, "%s: %s", fn, strerror(errno));
		return;
	}
	sn_clear();
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (sscanf(buf, "%d", &nid) == 1 &&
		    (n = node_for_nid(nid)) != NULL)
			sn_add(n, &fv_zero);
	}
	if (ferror(fp))
		warn("%s", fn);
	fclose(fp);
}

void
selnid_save(void)
{
	struct selnode *sn;
	const char *fn;
	FILE *fp;

	fn = _PATH_SELNIDS;
	if ((fp = fopen(fn, "w")) == NULL) {
		status_add(SLP_URGENT, "%s: %s", fn, strerror(errno));
		return;
	}
	SLIST_FOREACH(sn, &selnodes, sn_next)
		fprintf(fp, "%d\n", sn->sn_nodep->n_nid);
	if (ferror(fp))
		warn("%s", fn);
	fclose(fp);
}
