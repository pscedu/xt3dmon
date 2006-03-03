/* $Id$ */

#include "compat.h"	/* needed for vsnprintf */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int
vasprintf(char **ptr, const char *fmt, va_list ap)
{
	va_list apdup;
	int len;

//	va_copy(apdup, ap);
	apdup = ap;

	len = vsnprintf(NULL, 0, fmt, ap);
	if (len == -1)
			return (-1);

	len++;
	if ((*ptr = malloc(len)) == NULL)
		return (-1);

	len = vsnprintf(*ptr, len, fmt, ap);
//	if (len == -1)
//			return (-1);
	va_end(apdup);

	return (len);
}

int
asprintf(char **ptr, const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

//	if (len == -1)
//		return (-1);

	len++;
	if ((*ptr = malloc(len)) == NULL)
		return (-1);

	va_start(ap, fmt);
	len = vsnprintf(*ptr, len, fmt, ap);
	va_end(ap);

	return (len);
}
