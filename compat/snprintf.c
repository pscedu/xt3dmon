/* $Id$ */

#include "compat.h"

#include <stdarg.h>
#include <stdio.h>

#include "cdefs.h"

int
snprintf(char *buf, size_t sz, const char *fmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(buf, sz, fmt, ap);
	va_end(ap);
	return (len);
}


int
vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap)
{
	va_list save_ap = ap;
	int len;

	len = _vsnprintf(buf, sz, fmt, ap);

	if (len == -1) {
		buf[sz - 1] = '\0';
		len = _vscprintf(fmt, save_ap);
	}
	return (len);
}
