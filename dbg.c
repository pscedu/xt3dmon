/* $Id$ */

#include <stdarg.h>

#include "mon.h"

void
dbg_warn(const char *fmt, ...)
{
	va_list ap;

	if (!verbose)
		return;

	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);
}

void
dbg_crash(void)
{
	*(int *)NULL = 0;
}
