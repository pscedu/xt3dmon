/* $Id$ */

#ifndef _COMPAT_H_
#define _COMPAT_H_

#ifdef _MSC_VER

# define WIN32_LEAN_AND_MEAN
# define _USE_MATH_DEFINES

# include <stdarg.h>

# include <io.h>
# include <direct.h>
# include <windows.h>

# include <winsock2.h>
# include <ws2tcpip.h>

# include <GL/gl.h>
# include <GL/glut.h>

# include "getopt.h"
# include "dirent.h"

# define strcasecmp stricmp
# define random rand
# define srandom srand

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

# define round(a) floor((a) + 0.5)

# define usleep(x)

# define NAME_MAX BUFSIZ
# define PATH_MAX BUFSIZ
# define MAXHOSTNAMELEN 256

int asprintf(char **, const char *, ...);
int vasprintf(char **, const char *, va_list);
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, va_list);

char *strptime(const char *, const char *, struct tm *);
struct tm *gmtime_r(const time_t *, struct tm *);

int futimes(int, const struct timeval *);

int execvp(const char *, const char *const *);
int gettimeofday(struct timeval *, void *);
int ffs(int);
const char *basename(char *);
char *dirname(char *);

# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
# define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

/* Taken from sys/time.h */
# define timersub(a, b, result)						\
	do {								\
		(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
		(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;	\
		if ((result)->tv_usec < 0) {				\
			--(result)->tv_sec;				\
			(result)->tv_usec += 1000000;			\
		}							\
	} while (0)

# undef __inline
# define __inline

# undef __dead
# define __dead volatile

# undef SLIST_ENTRY

typedef UINT16 u_int16_t;
typedef UINT32 u_int32_t;
typedef signed int ssize_t;
typedef u_int16_t in_port_t;

# define mkdir(path, mode) mkdir(path)
# define SOCKETCLOSE(s) closesocket(s)

# define localtime_r(clock, result)	\
	do {				\
		struct tm *_tm;		\
					\
		_tm = localtime(clock);	\
		*result = *_tm;		\
	} while (0)

# define glutPostRedisplay post_redisplay
void post_redisplay(void);

# define glutMouseWheelFunc(a)

#elif defined(__APPLE_CC__)

# include <sys/param.h>
# include <sys/time.h>

# include <OpenGL/gl.h>
# include <GLUT/glut.h>

# include <dirent.h>
# include <unistd.h>
# include <netdb.h>

// # define port_t tcpip_port_t

#define glutMouseWheelFunc(a)

#define SOCKETCLOSE(s) close(s)

#else /* UNIX */

# if defined(__GNUC__) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE	/* asprintf */
# endif

# include <sys/param.h>
# include <sys/socket.h>
# include <sys/time.h>

# include <GL/gl.h>
# include <GL/freeglut.h>

# include <dirent.h>
# include <unistd.h>
# include <netdb.h>

#define SOCKETCLOSE(s) close(s)

#endif /* UNIX */

#ifndef EBADE
#define EBADE 1001
#endif

#endif /* _COMPAT_H_ */
