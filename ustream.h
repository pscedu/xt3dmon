/* $Id$ */

#define UST_FILE 0
#define UST_SOCK 1

struct ustream {
	int		 us_fd;
	int		 us_type;
	FILE		*us_fp;
	ssize_t		 usi_lastread;
	unsigned char	 usi_buf[BUFSIZ];
	unsigned char	*usi_bufstart;
	unsigned char	*usi_bufend;
};

struct ustream	*us_init(int, int, const char *);
int		 us_close(struct ustream *);
ssize_t		 us_write(struct ustream *, void *, size_t);
char		*us_gets(char *, int, struct ustream *);
int		 us_error(struct ustream *);
int		 us_eof(struct ustream *);
