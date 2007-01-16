/* $Id$ */

#include "compat.h"

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "ustream.h"
#include "util.h"

int
ustrop_winsock_init(const struct ustream *usp)
{
	return (1);
}

__inline int
ustrop_winsock_close(const struct ustream *usp)
{
	return (closesocket(usp->us_fd));
}

ssize_t
ustrop_winsock_write(struct ustream *usp, const void *buf, size_t siz)
{
	ssize_t rc;

	rc = send(usp->us_fd, buf, siz, 0);
	if (rc == -1)
		usp->us_error = rc;
	return (rc);
}

__inline ssize_t
my_recv(struct ustream *usp, size_t len)
{
	return (recv(usp->us_fd, usp->us_buf, len, 0));
}

__inline char *
ustrop_winsock_gets(struct ustream *usp, char *s, int siz)
{
	char *p, *endp;
	size_t len;
	long l;

	if (usp->us_flags & USF_HTTP_CHUNK && usp->us_chunksiz == 0) {
		char lenbuf[50];

 readlen:
		if (my_fgets(usp, lenbuf, sizeof(lenbuf),
		    my_recv) == NULL)
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

	p = my_fgets(usp, s, siz, my_recv);
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
ustrop_winsock_sawerror(const struct ustream *usp)
{
	return (usp->us_lastread == -1);
}

__inline const char *
ustrop_winsock_errstr(const struct ustream *usp)
{
	return (strerror(usp->us_error));
}

__inline int
ustrop_winsock_eof(const struct ustream *usp)
{
	return (usp->us_lastread == 0);
}

struct ustrdtab ustrdtab_winsock = {
	ustrop_winsock_init,
	ustrop_winsock_close,
	ustrop_winsock_write,
	ustrop_winsock_gets,
	ustrop_winsock_sawerror,
	ustrop_winsock_errstr,
	ustrop_winsock_eof
};
