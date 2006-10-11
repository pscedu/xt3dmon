/* $Id$ */

#include <stdarg.h>

#undef __dead
#define __dead volatile

__dead void	err(int, const char *, ...);
__dead void	errx(int, const char *, ...);
void	warn(const char *, ...);
void	warnx(const char *, ...);
void	vwarnx(const char *, va_list);
