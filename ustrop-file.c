/* $Id$ */

#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ustream.h"

struct ustrop ustrdtab_winsock[] = {
	ustrop_file_init,
	ustrop_file_close,
	ustrop_file_write,
	ustrop_file_gets,
	ustrop_file_error,
	ustrop_file_eof
};

struct ustream *
ustrop_file_init(int fd, int type, const char *modes)
{
	struct ustream *usp;

	if ((usp = malloc(sizeof(*usp))) == NULL)
		err(1, "malloc");
	memset(usp, 0, sizeof(*usp));

	usp->us_fd = fd;
	usp->us_type = type;

	if ((usp->us_fp = fdopen(fd, modes)) == NULL)
		err(1, "fdopen");

	/* Disable libc stream buffering to avoid blocking. */
	if (type == UST_SOCK)
		setbuf(usp->us_fp, NULL);

	return (usp);
}

__inline int
ustrop_file_close(struct ustream *usp)
{
	fclose(usp->us_fp);
}

/*
 * Yes, this shouldn't be here, but it's needed for sockets.
 */
__inline ssize_t
ustrop_file_write(struct ustream *usp, const void *buf, size_t siz)
{
	return (write(usp->us_fd, buf, siz));
}

__inline char *
ustrop_file_gets(char *s, int siz, struct ustream *usp)
{
	return (fgets(s, siz, usp->us_fp));
}

__inline int
ustrop_file_error(struct ustream *usp)
{
	return (ferror(usp->us_fp));
}

__inline int
ustrop_file_eof(struct ustream *usp)
{
	return (feof(usp->us_fp));
}
