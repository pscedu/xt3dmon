/* $Id$ */

struct ustream {
	int us_fd;
	union {
		FILE *us_fp;
	};
};

struct us *
us_init(int fd, const char *modes)
{
	struct us *usp;

	if ((usp = malloc(sizeof(*usp))) == NULL)
		err(1, "malloc");
	memset(usp, 0, sizeof(*usp));

	usp->us_fd = fd;

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
	return (write(usp->us_fp, buf, siz));
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
