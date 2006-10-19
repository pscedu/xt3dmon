/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "panel.h"
#include "queue.h"
#include "status.h"

struct status_hd status_hd = TAILQ_HEAD_INITIALIZER(status_hd);

void
status_pop(int prio)
{
	struct status_log *sl;

	TAILQ_FOREACH(sl, &status_hd, sl_link)
		if (sl->sl_prio == prio)
			break;
	if (sl == NULL) {
		warnx("status pop but no log with priority %d", prio);
		return;
	}

	TAILQ_REMOVE(&status_hd, sl, sl_link);
	free(sl->sl_str);
	free(sl);

	panel_rebuild(PANEL_STATUS);
	if (TAILQ_EMPTY(&status_hd))
		panel_hide(PANEL_STATUS);
}

void
status_add(int prio, const char *fmt, ...)
{
	struct status_log *sl;
	int duration;
	va_list ap;

	if ((sl = malloc(sizeof(*sl))) == NULL)
		err(1, "vasprintf");
	memset(sl, 0, sizeof(*sl));
	sl->sl_prio = prio;

	va_start(ap, fmt);
	if (vasprintf(&sl->sl_str, fmt, ap) == -1)
		err(1, "vasprintf");
	va_end(ap);

	TAILQ_INSERT_TAIL(&status_hd, sl, sl_link);

	duration = (prio == SLP_URGENT) ? 30 : 10;
	glutTimerFunc(duration * 1000, status_pop, prio);

	if (prio == SLP_URGENT)
		panel_show(PANEL_STATUS);
	panel_rebuild(PANEL_STATUS);
}
