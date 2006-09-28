/* $Id$ */

#include "mon.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "panel.h"

static char status_buf[BUFSIZ];

void
status_set(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(status_buf, sizeof(status_buf), fmt, ap);
	va_end(ap);
	panel_rebuild(PANEL_STATUS);
}

void
status_clear(void)
{
	status_buf[0] = '\0';
	panel_rebuild(PANEL_STATUS);
}

void
status_add(const char *fmt, ...)
{
	va_list ap;
	size_t len;

	len = strlen(status_buf);

	va_start(ap, fmt);
	vsnprintf(status_buf + len, sizeof(status_buf) - len, fmt, ap);
	va_end(ap);
	panel_rebuild(PANEL_STATUS);
}

__inline const char *
status_get(void)
{
	return (status_buf);
}
