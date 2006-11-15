/* $Id$ */

#if !defined __GNUC__ || __GNUC__ < 2
# ifndef __attribute__
# define __attribute__(x)
# endif
#endif

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

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

#ifndef howmany
#define howmany(x, y) (((x) + ((y) + 1)) / (y))
#endif

#ifndef NENTRIES
#define NENTRIES(t) (sizeof(t) / sizeof(t[0]))
#endif

#if defined(__FLT_MAX__) && !defined(FLT_MAX)
#define FLT_MAX __FLT_MAX__
#endif

#if 0
#undef __inline
#define __inline
#endif
