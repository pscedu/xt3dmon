#include <stdarg.h>


void	err(int, const char *, ...);
void	errx(int, const char *, ...);
void	warn(const char *, ...);
void	warnx(const char *, ...);
void	vwarnx(const char *, va_list);
