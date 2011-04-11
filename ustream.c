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
 * Yes, this shouldn't be here, but it's needed for socket
 * communication.
 */
__inline ssize_t
us_write(struct ustream *usp, const void *buf, size_t siz)
{
	return (usp->us_dtab->ust_write(usp, buf, siz));
}

__inline char *
us_gets(struct ustream *usp, char *s, int siz)
{
	return (usp->us_dtab->ust_gets(usp, s, siz));
}

__inline int
us_sawerror(const struct ustream *usp)
{
	return (usp->us_dtab->ust_sawerror(usp));
}

__inline const char *
us_errstr(const struct ustream *usp)
{
	return (usp->us_dtab->ust_errstr(usp));
}

__inline int
us_eof(const struct ustream *usp)
{
	return (usp->us_dtab->ust_eof(usp));
}
