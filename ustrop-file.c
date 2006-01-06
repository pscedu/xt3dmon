/* $Id$ */

/*
 * This file is worthless.
 *
 * This universal stream interface was created so fgets()
 * can be used on winsocks.  Since UNIX provides the same
 * syscalls on all descriptors (not special ones for sockets),
 * nothing is really needed here.
 */

#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

	if ((usp->us_fp = fdopen(fd, modes)) == NULL)
		err(1, "fdopen");

	return (usp);
}

int
us_close(struct ustream *usp)
{
	FILE *fp;

	fp = usp->us_fp;
	free(usp);			/* XXX: errno */
	return (fclose(fp));
}

/*
 * Yes, this shouldn't be here, but it's needed for sockets.
 */
ssize_t
us_write(struct ustream *usp, void *buf, size_t siz)
{
	return (write(usp->us_fd, buf, siz));
}

char *
us_gets(char *s, int siz, struct ustream *usp)
{
	return (fgets(s, siz, usp->us_fp));
}

int
us_error(struct ustream *usp)
{
	return (ferror(usp->us_fp));
}

int
us_eof(struct ustream *usp)
{
	return (feof(usp->us_fp));
}
