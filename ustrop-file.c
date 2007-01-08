/* $Id$ */

#include "compat.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ustream.h"

int
ustrop_file_init(struct ustream *usp)
{
	if ((usp->us_fp = fdopen(usp->us_fd, usp->us_modes)) == NULL)
		/* return (0); */
		err(1, "fdopen");

	/* Disable libc stream buffering to avoid blocking. */
	if (usp->us_type == UST_REMOTE)
		setbuf(usp->us_fp, NULL);
	return (1);
}

__inline int
ustrop_file_close(const struct ustream *usp)
{
	return (fclose(usp->us_fp));
}

/*
 * Yes, this shouldn't be here, but it's needed for sockets.
 */
__inline ssize_t
ustrop_file_write(struct ustream *usp, const void *buf, size_t siz)
{
	ssize_t rc;

	rc = write(usp->us_fd, buf, siz);
	usp->us_error = errno;
	return (rc);
}

__inline char *
ustrop_file_gets(struct ustream *usp, char *s, int siz)
{
	char *p;

	p = fgets(s, siz, usp->us_fp);
	usp->us_error = errno;
	return (p);
}

__inline int
ustrop_file_sawerror(const struct ustream *usp)
{
	return (ferror(usp->us_fp));
}

__inline const char *
ustrop_file_errstr(const struct ustream *usp)
{
	return (strerror(usp->us_error));
}

__inline int
ustrop_file_eof(const struct ustream *usp)
{
	return (feof(usp->us_fp));
}

struct ustrdtab ustrdtab_file = {
	ustrop_file_init,
	ustrop_file_close,
	ustrop_file_write,
	ustrop_file_gets,
	ustrop_file_sawerror,
	ustrop_file_errstr,
	ustrop_file_eof
};
