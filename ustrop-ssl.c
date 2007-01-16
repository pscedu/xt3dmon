/* $Id$ */

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
ustrop_ssl_errstr(__unused const struct ustream *usp)
{
	return (NULL);
}

__inline int
ustrop_ssl_eof(const struct ustream *usp)
{
	return (usp->us_lastread == 0);
}

struct ustrdtab ustrdtab_ssl = {
	ustrop_ssl_init,
	ustrop_ssl_close,
	ustrop_ssl_write,
	ustrop_ssl_gets,
	ustrop_ssl_sawerror,
	ustrop_ssl_errstr,
	ustrop_ssl_eof
};
