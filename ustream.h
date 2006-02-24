/* $Id$ */

#define UST_LOCAL	0
#define UST_REMOTE	1
#define UST_SSL		2

struct ustream {
	int		 us_fd;
	FILE		*us_fp;
	ssize_t		 us_lastread;
	unsigned char	 us_buf[BUFSIZ];
	unsigned char	*us_bufstart;
	unsigned char	*us_bufend;
};

struct ustrdtab {
	struct ustream	*(*ust_init)(int, int, const char *);
	int		 (*ust_close)(const struct ustream *);
	ssize_t		 (*ust_write)(const struct ustream *, const void *, size_t);
	char		*(*ust_gets)(char *, int, const struct ustream *);
	int		 (*ust_error)(const struct ustream *);
	int		 (*ust_eof)(const struct ustream *);
};

struct ustream	*us_init(int, int, const char *);
int		 us_close(struct ustream *);
ssize_t		 us_write(const struct ustream *, const void *, size_t);
char		*us_gets(const struct ustream *, char *, int);
int		 us_error(const struct ustream *);
int		 us_eof(const struct ustream *);
