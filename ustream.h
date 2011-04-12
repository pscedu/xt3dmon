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

#include "mon.h"

#include <stdio.h>

#include <openssl/ssl.h>

#define UST_LOCAL	0
#define UST_REMOTE	1
#define UST_SSL		2
#define UST_GZIP	3
#define NUST		4

struct ustrdtab;

struct ustream {
	struct ustrdtab	*us_dtab;
	int		 us_type;
	const char	*us_modes;

	int		 us_fd;
	int		 us_error;
	FILE		*us_fp;
	void		*us_zfp;
	ssize_t		 us_lastread;
	char		 us_buf[BUFSIZ];
	char		*us_bufstart;
	char		*us_bufend;
	SSL_CTX		*us_ctx;
	SSL		*us_ssl;
	int		 us_flags;	/* ustream-specific flags */

	/* HTTP-specific XXX put in union */
	unsigned int	 us_chunksiz;
};

#define USF_USR1	(1 << 0)

#define USF_HTTP_CHUNK	USF_USR1	/* length-prefixed chunks */

struct ustrdtab {
	int		 (*ust_init)(struct ustream *);
	int		 (*ust_close)(const struct ustream *);
	ssize_t		 (*ust_write)(struct ustream *, const void *, size_t);
	char		*(*ust_gets)(struct ustream *, char *, int);
	int		 (*ust_sawerror)(const struct ustream *);
	const char	*(*ust_errstr)(const struct ustream *);
	int		 (*ust_eof)(const struct ustream *);
	int		 (*ust_getc)(struct ustream *);
	void		 (*ust_ungetc)(struct ustream *, int);
};

struct ustream	*us_init(int, int, const char *);
int		 us_close(struct ustream *);
ssize_t		 us_write(struct ustream *, const void *, size_t);
char		*us_gets(struct ustream *, char *, int);
int		 us_sawerror(const struct ustream *);
const char	*us_errstr(const struct ustream *);
int		 us_eof(const struct ustream *);

/*
 * Yes, this shouldn't be here, but it's needed for socket
 * communication.
 */
#define us_write(usp, buf, siz)		(usp)->us_dtab->ust_write((usp), (buf), (siz))
#define us_gets(usp, s, siz)		(usp)->us_dtab->ust_gets((usp), (s), (siz))
#define us_sawerror(usp)		(usp)->us_dtab->ust_sawerror(usp)
#define us_errstr(usp)			(usp)->us_dtab->ust_errstr(usp)
#define us_eof(usp)			(usp)->us_dtab->ust_eof(usp)
#define us_getc(usp)			(usp)->us_dtab->ust_getc(usp)
#define us_ungetc(usp, c)		(usp)->us_dtab->ust_ungetc(usp, c)

extern struct ustrdtab	*ustrdtabs[NUST];
