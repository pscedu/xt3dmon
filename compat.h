/* $Id$ */

#ifndef _COMPAT_H
#define _COMPAT_H

#ifdef _MSC_VER

#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES

# include <stdarg.h>

# include <io.h>
# include <direct.h>
# include <windows.h>

# include <winsock2.h>
# include <ws2tcpip.h>

# include <GL/gl.h>
# include <GL/glut.h>

# include "getopt.h"

# define atanf atan
# define acosf acos
# define asinf asin
# define cosf cos
# define sinf sin
# define ceilf ceil
# define floorf floor
# define log10f log10

# define write _write
# define read _read

#define round(a) floor((a) + 0.5)

# define NAME_MAX BUFSIZ
# define PATH_MAX BUFSIZ

int asprintf(char **, const char *, ...);
int vasprintf(char **, const char *, va_list);
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, va_list);

int execvp(const char *, const char *const *);
int gettimeofday(struct timeval *, void *);
int ffs(int);

/* Taken from sys/time.h */
# define timersub(a, b, result)                                             \
do {                                                                        \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                       \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                    \
	if ((result)->tv_usec < 0) {                                        \
		--(result)->tv_sec;                                         \
		(result)->tv_usec += 1000000;                               \
	}                                                                   \
} while (0)

# undef __inline
# define __inline

#undef SLIST_ENTRY

typedef UINT16 u_int16_t;
typedef signed int ssize_t;
typedef u_int16_t in_port_t;

#define mkdir(path, mode) mkdir(path)

#define localtime_r(clock, result)	\
	do {				\
		struct tm *_tm;		\
					\
		_tm = localtime(clock);	\
		*result = *_tm;		\
	} while (0)

#elif defined(__APPLE_CC__)

# include <sys/param.h>
# include <sys/time.h>

# include <OpenGL/gl.h>
# include <GLUT/glut.h>

# include <unistd.h>
# include <netdb.h>

// # define port_t tcpip_port_t

#else /* UNIX */

# if defined(__GNUC__) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE	/* asprintf */
# endif

# include <sys/param.h>
# include <sys/socket.h>
# include <sys/time.h>

# include <GL/gl.h>
# include <GL/freeglut.h>

# include <unistd.h>
# include <netdb.h>

#endif

#endif /* _COMPAT_H */
