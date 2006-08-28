/* $Id$ */

#include "compat.h"

#include <err.h>
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
ustrop_ssl_write(const struct ustream *usp, const void *buf, size_t siz)
{
	return (SSL_write(usp->us_ssl, buf, siz));
}

char *
ustrop_ssl_gets(struct ustream *usp, char *s, int siz)
{
	size_t total, chunksiz;
	char *ret, *nl, *endp;
	int remaining;
	ssize_t nr;

	remaining = siz - 1;		/* NUL termination. */
	total = 0;
	ret = s;
	while (remaining > 0) {
		/* Look for newline in current buffer. */
		if (usp->us_bufstart) {
			if ((nl = strnchr(usp->us_bufstart, '\n',
			    usp->us_bufend - usp->us_bufstart + 1)) != NULL)
				endp = nl;
			else
				endp = usp->us_bufend;
			chunksiz = MIN(endp - usp->us_bufstart + 1, remaining);

			/* Copy all data up to any newline. */
			memcpy(s + total, usp->us_bufstart, chunksiz);
			remaining -= chunksiz;
			total += chunksiz;
			usp->us_bufstart += chunksiz;
			if (usp->us_bufstart > usp->us_bufend)
				usp->us_bufstart = NULL;
			if (nl)
				break;
		}

		/* Not found, read more. */
		chunksiz = MIN(remaining, (int)sizeof(usp->us_buf));
		nr = SSL_read(usp->us_ssl, usp->us_buf, chunksiz);
		usp->us_lastread = nr;

		if (nr == -1 || nr == 0)
			return (NULL);

		usp->us_bufstart = usp->us_buf;
		usp->us_bufend = usp->us_buf + nr - 1;
	}
	/*
	 * This should be safe because total is
	 * bound by siz - 1.
	 */
	s[total] = '\0';
	return (ret);
}

__inline int
ustrop_ssl_error(const struct ustream *usp)
{
	return (usp->us_lastread == -1);
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
	ustrop_ssl_error,
	ustrop_ssl_eof
};
