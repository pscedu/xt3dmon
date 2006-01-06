/* $Id$ */

#define UST_FILE 0
#define UST_SOCK 1

struct ustream {
	int		 us_fd;
	int		 us_type;
	FILE		*us_fp;
	ssize_t		 us_lastread;
	unsigned char	 us_buf[BUFSIZ];
	unsigned char	*us_bufstart;
	unsigned char	*us_bufend;
};

struct ustream	*us_init(int, int, const char *);
int		 us_close(struct ustream *);
ssize_t		 us_write(struct ustream *, const void *, size_t);
char		*us_gets(char *, int, struct ustream *);
int		 us_error(struct ustream *);
int		 us_eof(struct ustream *);
