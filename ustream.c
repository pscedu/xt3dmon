/* $Id$ */

#include "mon.h"

#include <sys/types.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "ustream.h"

struct ustream *
us_init(int fd, int type, const char *modes)
{
	struct ustream *usp;

	if ((usp = malloc(sizeof(*usp))) == NULL)
		err(1, "malloc");
	memset(usp, 0, sizeof(*usp));

	usp->us_fd = fd;
	usp->us_type = type;
	usp->us_modes = modes;
	usp->us_dtab = ustrdtabs[type];

	if (usp->us_dtab->ust_init(usp))
		return (usp);
	free(usp);			/* XXX: errno */
	return (NULL);
}

int
us_close(struct ustream *usp)
{
	int ret;

	ret = usp->us_dtab->ust_close(usp);
	free(usp);			/* XXX: errno */
	return (ret);
}

/*
 * Yes, this shouldn't be here, but it's needed
 * for socket communication.
 */
__inline ssize_t
us_write(const struct ustream *usp, const void *buf, size_t siz)
{
	return (usp->us_dtab->ust_write(usp, buf, siz));
}

__inline char *
us_gets(struct ustream *usp, char *s, int siz)
{
	return (usp->us_dtab->ust_gets(usp, s, siz));
}

__inline int
us_error(const struct ustream *usp)
{
	return (usp->us_dtab->ust_error(usp));
}

__inline int
us_eof(const struct ustream *usp)
{
	return (usp->us_dtab->ust_eof(usp));
}
