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

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE	/* glibc2 */
# include <time.h>
# undef _XOPEN_SOURCE
#else
# include <time.h>
#endif

struct ustream;

struct http_req {
	const char	 *htreq_server;
	const char	 *htreq_port;		/* host-byte order. */
	int		  htreq_flags;

	const char	 *htreq_method;
	const char	 *htreq_version;
	const char	 *htreq_url;
	char		**htreq_extra;		/* NULL-terminate. */
	void		(*htreq_extraf)(struct ustream *);
};

struct http_res {
	int		  htres_code;
	struct tm	  htres_mtime;
};

struct ustream	*http_open(struct http_req *, struct http_res *);
