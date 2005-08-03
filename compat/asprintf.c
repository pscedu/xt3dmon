/* $Id$ */

#include "compat.h"	/* needed for vsnprintf */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


int
asprintf(char **ptr, const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	if ((*ptr = malloc(len)) == NULL)
		return (-1);

	va_start(ap, fmt);
	vsnprintf(*ptr, len, fmt, ap);
	va_end(ap);

	return (0);
}
