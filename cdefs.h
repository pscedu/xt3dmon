/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#if !defined __GNUC__ || __GNUC__ < 2
# ifndef __attribute__
# define __attribute__(x)
# endif
#endif

#ifndef __unusedx
# define __unusedx __attribute__((__unused__))
#endif

#define MID(max, min)	(((max) + (min)) / 2)

#ifndef MIN
# define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef MIN3
# define MIN3(a, b, c)	MIN(MIN((a), (b)), (c))
#endif

#ifndef MAX
# define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#ifndef MAX3
# define MAX3(a, b, c)	MAX(MAX((a), (b)), (c))
#endif

#undef howmany
#define howmany(x, y)	(((x) + (y) + 1) / (y))

#ifndef nitems
# define nitems(t)	(sizeof(t) / sizeof(t[0]))
#endif

#if defined(__FLT_MAX__) && !defined(FLT_MAX)
# define FLT_MAX __FLT_MAX__
#endif

#if 0
#undef __inline
#define __inline
#endif
