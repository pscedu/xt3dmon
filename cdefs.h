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
