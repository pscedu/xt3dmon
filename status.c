/* $Id$ */

#include <stdarg.h>
#include <stdio.h>

#include "mon.h"

static char status_buf[BUFSIZ];

void
status_set(const char *fmt, ...)
{
	struct panel *p;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(status_buf, sizeof(status_buf), fmt, ap);
	va_end(ap);

	if ((p = panel_for_id(PANEL_STATUS)) != NULL)
		p->p_opts |= POPT_DIRTY;
}

void
status_clear(void)
{
	struct panel *p;

	status_buf[0] = '\0';
	if ((p = panel_for_id(PANEL_STATUS)) != NULL)
		p->p_opts |= POPT_DIRTY;
}

void
status_add(const char *fmt, ...)
{
	struct panel *p;
	va_list ap;
	int len;

	len = strlen(status_buf);

	va_start(ap, fmt);
	vsnprintf(status_buf + len, sizeof(status_buf) - len, fmt, ap);
	va_end(ap);

	if ((p = panel_for_id(PANEL_STATUS)) != NULL)
		p->p_opts |= POPT_DIRTY;
}

__inline const char *
status_get(void)
{
	return (status_buf);
}
