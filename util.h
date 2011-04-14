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

#include <sys/types.h>

struct ustream;

/* 1000.00KB */
#define FMT_SCALED_BUFSIZ	12

int		 baseconv(int);
void		 base64_encode(const void *, char *, size_t);
char		*strnchr(const char *, char, size_t);
const char	*smart_dirname(const char *);
const char	*smart_basename(const char *);
void		 escape_printf(struct buf *, const char *);
void		 fmt_scaled(size_t, char *);
char *		 my_fgets(struct ustream *, char *, int,
		    ssize_t (*)(struct ustream *, size_t));
