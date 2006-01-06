/* $Id$ */

#include "compat.h"	/* needed for vsnprintf */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int
vasprintf(char **ptr, char *fmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	if (len == -1)
			return (-1);

	len++;
	if ((*ptr = malloc(len)) == NULL)
		return (-1);

	va_start(ap, fmt);
	len = vsnprintf(*ptr, len, fmt, ap);
	va_end(ap);

	return (0);
}

int
asprintf(char **ptr, const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	len++;
	if ((*ptr = malloc(len)) == NULL)
		return (-1);

	va_start(ap, fmt);
	vsnprintf(*ptr, len, fmt, ap);
	va_end(ap);

	return (0);
}
