/* $Id$ */

#if !defined __GNUC__ || __GNUC__ < 2
# define __attribute__(x) 
#endif

#define __unused __attribute__((__unused__))

#define MID(max, min) (((max) + (min)) / 2.0)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MIN3
#define MIN3(a, b, c) MIN(MIN((a), (b)), (c))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MAX3
#define MAX3(a, b, c) MAX(MAX((a), (b)), (c))
#endif

#if defined(__FLT_MAX__) && !defined(FLT_MAX)
#define FLT_MAX __FLT_MAX__
#endif

#if 0
#undef __inline
#define __inline
#endif
