/* $Id$ */

#ifndef _COMPAT_H
#define _COMPAT_H

#ifdef _MSC_VER
# include <stdarg.h>

# include <windows.h>

# include <GL/gl.h>
# include <GL/glut.h>

# include "getopt.h"

# define acosf acos
# define asinf asin
# define ceilf ceil
# define floorf floor
# define log10f log10

# define NAME_MAX BUFSIZ
# define PATH_MAX BUFSIZ

int asprintf(char **, const char *, ...);
int ffs(int);
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, va_list);

# undef __inline
# define __inline

#undef SLIST_ENTRY

#elif defined(__APPLE_CC__)

# include <sys/param.h>

# include <OpenGL/gl.h>
# include <GLUT/glut.h>

#else /* UNIX */

# if defined(__GNUC__) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE	/* asprintf */
# endif

# include <sys/param.h>

# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#endif /* _COMPAT_H */
