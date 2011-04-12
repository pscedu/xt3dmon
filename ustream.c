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
