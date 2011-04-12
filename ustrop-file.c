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
	size_t len;
	char *p;
	char c;

	if (usp->us_type == UST_REMOTE && usp->us_flags & USF_HTTP_CHUNK &&
	    usp->us_chunksiz == 0) {
		if (fscanf(usp->us_fp, "%x", &usp->us_chunksiz) != 1) {
			warnx("cannot read chunk size");
			usp->us_error = EBADE; /* XXX */
			return (NULL);
		}
		/* Test if at EOF. */
		if ((c = fgetc(usp->us_fp)) != '\r')
			ungetc(c, usp->us_fp);
		else if ((c = fgetc(usp->us_fp)) != '\n')
			ungetc(c, usp->us_fp);
		else if ((c = fgetc(usp->us_fp)) != '\r')
			ungetc(c, usp->us_fp);
		else if ((c = fgetc(usp->us_fp)) != '\n')
			ungetc(c, usp->us_fp);
		else if ((c = fgetc(usp->us_fp)) != EOF)
			ungetc(c, usp->us_fp);
		if (usp->us_chunksiz == 0)
			return (NULL);
	}

	p = fgets(s, siz, usp->us_fp);
	usp->us_error = errno;

	if (usp->us_type == UST_REMOTE && usp->us_flags & USF_HTTP_CHUNK) {
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

__inline int
ustrop_file_getc(struct ustream *usp)
{
	return (fgetc(usp->us_fp));
}

__inline void
ustrop_file_ungetc(struct ustream *usp, int c)
{
	return (ungetc(c, usp->us_fp));
}

struct ustrdtab ustrdtab_file = {
	ustrop_file_init,
	ustrop_file_close,
	ustrop_file_write,
	ustrop_file_gets,
	ustrop_file_sawerror,
	ustrop_file_errstr,
	ustrop_file_eof,
	ustrop_file_getc,
	ustrop_file_ungetc
};
