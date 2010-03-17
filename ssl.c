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

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "xssl.h"

const char *
ssl_errstr(int code)
{
	switch (code) {
	case SSL_ERROR_NONE:
		return ("none");
	case SSL_ERROR_ZERO_RETURN:
		return ("zero return");
	case SSL_ERROR_WANT_READ:
		return ("want read");
	case SSL_ERROR_WANT_WRITE:
		return ("want write");
	case SSL_ERROR_WANT_CONNECT:
		return ("want connect");
	case SSL_ERROR_WANT_ACCEPT:
		return ("want accept");
	case SSL_ERROR_WANT_X509_LOOKUP:
		return ("want lookup");
	case SSL_ERROR_SYSCALL:
		return ("syscall");
	case SSL_ERROR_SSL:
		return ("ssl lib error");
	default:
		return ("unknown");
	}
}

const char *
ssl_error(SSL *ssl, int code)
{
#define ERRBUF_LEN 120
	static char sslerr[ERRBUF_LEN];
	static char errbuf[BUFSIZ];

	int xcode = SSL_get_error(ssl, code);

	if (ERR_error_string(ERR_get_error(), sslerr) == NULL)
		return ("unable to get SSL error");
	snprintf(errbuf, sizeof(errbuf), "ssl: %s (%s)", sslerr,
	    ssl_errstr(xcode));
	return (errbuf);
}

void
ssl_init(void)
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
}
