/* $Id$ */

#define USTROP_INIT	0
#define USTROP_CLOSE	1
#define USTROP_WRITE	2
#define USTROP_GETS	3
#define USTROP_ERROR	4
#define USTROP_EOF	5

struct ustream *
us_init(int fd, int type, const char *modes)
{
	struct ustream *usp;

	if ((usp = malloc(sizeof(*usp))) == NULL)
		err(1, "malloc");
	memset(usp, 0, sizeof(*usp));

	usp->us_fd = fd;
	usp->us_dtab = us_dtab[type];

	return ((*usp->us_dtab[USTROP_INIT])(usp));
}

int
us_close(struct ustream *usp)
{
	int error;

	ret = (*usp->us_dtab[USTROP_CLOSE])(usp);
	free(usp);			/* XXX: errno */
	return (ret);
}

/*
 * Yes, this shouldn't be here, but it's needed
 * for socket communication.
 */
__inline ssize_t
us_write(struct ustream *usp, const void *buf, size_t siz)
{
	return ((*usp->us_dtab[USTROP_WRITE])(usp));
}

__inline char *
us_gets(char *s, int siz, struct ustream *usp)
{
	return ((*usp->us_dtab[USTROP_GETS])(usp));
}

__inline int
us_error(struct ustream *usp)
{
	return ((*usp->us_dtab[USTROP_ERROR])(usp));
}

__inline int
us_eof(struct ustream *usp)
{
	return ((*usp->us_dtab[USTROP_EOF])(usp));
}
