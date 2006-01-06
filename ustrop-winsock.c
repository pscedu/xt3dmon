/* $Id$ */

struct us *
us_init(int fd, int type)
{
	struct us *usp;

	if ((usp = malloc(sizeof(*usp))) == NULL)
		err(1, "malloc");
	memset(usp, 0, sizeof(*usp));

	usp->us_fd = fd;
	usp->us_type = type;

	return (usp);
}

int
us_close(struct ustream *usp)
{
	int fd, ret;
	FILE *fp;

	fp = usp->us_fp;
	fd = usp->us_fd;
	free(usp);

	switch (usp->us_type) {
	case UST_FILE:
		ret = fclose(fp);
		break;
	case UST_SOCK:
		ret = closesock(fd);
		break;
	}
	return (ret);
}

ssize_t
us_write(struct ustream *usp, void *buf, size_t siz)
{
	ssize_t written;

	written = 0; /* gcc */
	switch (usp->us_type) {
	case UST_FILE:
errx(1, "unimplemented");
//		written = write(usp->us_fd, buf, siz);
		break;
	case UST_SOCK:
		written = send(usp->us_fd, buf, siz);
		break;
	}
	return (written);
}

char *
strnchr(char *s, char c, size_t len)
{
	size_t pos;

	for (pos = 0; s[pos] != c && pos < len; pos++)
		;

	if (pos == len)
		return (NULL);
	return (s + pos);
}

char *
us_gets(char *s, int siz, struct ustream *usp)
{
	size_t total, chunksiz;
	char *ret, *nl, *endp;
	int remaining;
	ssize_t nr;

	remaining = siz - 1;		/* NUL termination. */
	total = 0;
	ret = NULL; /* gcc */
	switch (usp->us_type) {
	case UST_FILE:
		ret = fgets(s, siz, usp->usp_fp);
		break;
	case UST_SOCK:
		ret = s;
		while (remaining > 0) {
			/* Look for newline in current buffer. */
			if (usp->us_bufstart) {
				if ((nl = strnchr(usp->us_bufstart, '\n',
				    usp->us_bufend - usp->us_bufstart)) != NULL)
					endp = nl;
				else
					endp = usp->us_bufend;
				chunksiz = MIN(endp - usp->us_startpos, remaining);

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
			chunksiz = MIN(remaining, sizeof(buf));
			nr = recv(usp->us_fd, buf, chunksiz);
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
		s[total - 1] = '\0';
		break;
	}
	return (ret);
}

__inline int
us_error(struct ustream *usp)
{
	return (usp->us_lastread == -1);
}

__inline int
us_eof(struct ustream *usp)
{
	return (usp->us_lastread == 0);
}