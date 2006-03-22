/* $Id$ */

struct ustream;

struct http_req {
	const char	 *htreq_server;
	in_port_t	  htreq_port;		/* host-byte order. */
	int		  htreq_flags;

	const char	 *htreq_method;
	const char	 *htreq_version;
	const char	 *htreq_url;
	char		**htreq_extra;		/* NULL-terminate. */
};

struct http_res {
	int		  htres_code;
};

struct ustream	*http_open(struct http_req *, struct http_res *);
