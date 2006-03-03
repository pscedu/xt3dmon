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
ustrop_winsock_write(const struct ustream *usp, const void *buf, size_t siz)
{
	return (send(usp->us_fd, buf, siz, 0));
}

char *
ustrop_winsock_gets(struct ustream *usp, char *s, int siz)
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
		chunksiz = MIN(remaining, sizeof(usp->us_buf));
		nr = recv(usp->us_fd, usp->us_buf, chunksiz, 0);
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
ustrop_winsock_error(const struct ustream *usp)
{
	return (usp->us_lastread == -1);
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
