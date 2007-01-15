/* $Id$ */

#include "compat.h"

#include <err.h>
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
		usp->usp_error = rc;
	return (rc);
}

__inline ssize_t
my_recv(struct ustream *usp, size_t len)
{
	return (recv(usp->us_ssl, usp->us_buf, len, 0));
}

__inline char *
ustrop_winsock_gets(struct ustream *usp, char *s, int siz)
{
	return (my_fgets(usp, s, siz, my_recv));
}

__inline int
ustrop_winsock_sawerror(const struct ustream *usp)
{
	return (usp->us_lastread == -1);
}

__inline const char *
ustrop_winsock_errstr(const struct ustream *usp)
{
	return (strerror(usp->usp_error));
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
	ustrop_winsock_error,
	ustrop_winsock_eof
};
