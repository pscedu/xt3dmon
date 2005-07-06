/* $Id$ */

#if !defined __GNUC__ || __GNUC__ < 2
# define __attribute__(x) 
#endif

#define __unused __attribute__((__unused__))

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#if defined(__FLT_MAX__) && !defined(FLT_MAX)
#define FLT_MAX __FLT_MAX__
#endif

#if 0
#undef __inline
#define __inline
#endif
