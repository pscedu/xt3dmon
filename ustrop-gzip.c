/* $Id$ */

#include "compat.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "ustream.h"
#include "cdefs.h"

int
ustrop_gzip_init(struct ustream *usp)
{
	if ((usp->us_zfp = gzdopen(usp->us_fd, usp->us_modes)) == NULL)
		/* return (0); */
		err(1, "fdopen");

#if 0
	/* Disable libc stream buffering to avoid blocking. */
	if (usp->us_type == UST_REMOTE)
		setbuf(usp->us_zfp, NULL);
#endif
	return (1);
}

__inline int
ustrop_gzip_close(const struct ustream *usp)
{
	return (gzclose(usp->us_zfp));
}

__inline ssize_t
ustrop_gzip_write(__unused struct ustream *usp,
    __unused const void *buf, __unused size_t siz)
{
	errx(1, "not supported"); /* XXX */
}

__inline char *
ustrop_gzip_gets(struct ustream *usp, char *s, int siz)
{
	return (gzgets(usp->us_zfp, s, siz));
}

__inline int
ustrop_gzip_sawerror(const struct ustream *usp)
{
	int error;

	gzerror(usp->us_zfp, &error);
	return (error != Z_OK && error != Z_STREAM_END);
}

__inline const char *
ustrop_gzip_errstr(const struct ustream *usp)
{
	int error;

	return (gzerror(usp->us_zfp, &error));
}

__inline int
ustrop_gzip_eof(const struct ustream *usp)
{
	return (gzeof(usp->us_zfp));
}

struct ustrdtab ustrdtab_gzip = {
	ustrop_gzip_init,
	ustrop_gzip_close,
	ustrop_gzip_write,
	ustrop_gzip_gets,
	ustrop_gzip_sawerror,
	ustrop_gzip_errstr,
	ustrop_gzip_eof
};
