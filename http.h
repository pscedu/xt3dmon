/* $Id$ */

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
	void		(*htreq_extraf)(const struct ustream *);
};

struct http_res {
	int		  htres_code;
	struct tm	  htres_mtime;
};

struct ustream	*http_open(struct http_req *, struct http_res *);
