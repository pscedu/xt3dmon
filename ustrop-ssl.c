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

#include "compat.h"

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>

#include "cdefs.h"
#include "ustream.h"
#include "util.h"
#include "xssl.h"

int
ustrop_ssl_init(struct ustream *usp)
{
	int ret;

	if ((usp->us_ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
		errx(1, "%s", ssl_error(NULL, 0));
	usp->us_ssl = SSL_new(usp->us_ctx);
	if (usp->us_ssl == NULL)
		errx(1, "%s", ssl_error(NULL, 0));
	SSL_set_fd(usp->us_ssl, usp->us_fd);

	if ((ret = SSL_connect(usp->us_ssl)) != 1)
		errx(1, "%s", ssl_error(usp->us_ssl, ret));
	return (1);
}

__inline int
ustrop_ssl_close(const struct ustream *usp)
{
	SSL_free(usp->us_ssl);
	SSL_CTX_free(usp->us_ctx);
	return (SOCKETCLOSE(usp->us_fd));
}

ssize_t
ustrop_ssl_write(struct ustream *usp, const void *buf, size_t siz)
{
	return (SSL_write(usp->us_ssl, buf, siz));
}

__inline ssize_t
my_ssl_read(struct ustream *usp, size_t len)
{
	return (SSL_read(usp->us_ssl, usp->us_buf, len));
}

char *
ustrop_ssl_gets(struct ustream *usp, char *s, int siz)
{
	char *p, *endp;
	size_t len;
	long l;

	if (usp->us_flags & USF_HTTP_CHUNK && usp->us_chunksiz == 0) {
		char lenbuf[50];

 readlen:
		if (my_fgets(usp, lenbuf, sizeof(lenbuf),
		    my_ssl_read) == NULL)
			return (NULL);
		if ((l = strtol(lenbuf, &endp, 16)) < 0 || l > INT_MAX) {
			errno = EBADE;
			return (NULL);
		}
		if (lenbuf == endp)
			goto readlen;
		usp->us_chunksiz = (int)l;
		if (usp->us_chunksiz == 0)
			return (NULL);
	}

	p = my_fgets(usp, s, siz, my_ssl_read);
	if (usp->us_flags & USF_HTTP_CHUNK) {
		len = strlen(s);
		if (usp->us_chunksiz < len) {
			warnx("error: chunk exceeds length (have=%d, want=%d)",
			    usp->us_chunksiz, len);
			usp->us_chunksiz = 0;
		} else
			usp->us_chunksiz -= len;
	}
	return (p);
}

__inline int
ustrop_ssl_sawerror(const struct ustream *usp)
{
	return (usp->us_lastread == -1);
}

__inline const char *
ustrop_ssl_errstr(__unusedx const struct ustream *usp)
{
	return (NULL);
}

__inline int
ustrop_ssl_eof(const struct ustream *usp)
{
	return (usp->us_lastread == 0);
}

__inline int
ustrop_ssl_getc(const struct ustream *usp)
{
	int c;

	if (ustrop_ssl_eof(usp) || ustrop_ssl_sawerror(usp))
		return (EOF);
	p = ustrop_ssl_gets(usp, &c, 1);
	return (p ? c : EOF);
}

__inline void
ustrop_ssl_ungetc(const struct ustream *usp, int c)
{
	if (usp->us_bufstart == usp->us_buf)
		errx(1, "unable to put back to before buffer start");
	*--usp->us_bufstart = c;
}

struct ustrdtab ustrdtab_ssl = {
	ustrop_ssl_init,
	ustrop_ssl_close,
	ustrop_ssl_write,
	ustrop_ssl_gets,
	ustrop_ssl_sawerror,
	ustrop_ssl_errstr,
	ustrop_ssl_eof,
	ustrop_ssl_getc,
	ustrop_ssl_ungetc
};
