/* $Id$ */

#include "compat.h"

#include <stdio.h>

#include <openssl/ssl.h>

#define UST_LOCAL	0
#define UST_REMOTE	1
#define UST_SSL		2
#define NUST		3

struct ustrdtab;

struct ustream {
	struct ustrdtab	*us_dtab;
	int		 us_type;
	const char	*us_modes;

	int		 us_fd;
	FILE		*us_fp;
	ssize_t		 us_lastread;
	char		 us_buf[BUFSIZ];
	char		*us_bufstart;
	char		*us_bufend;
	SSL_CTX		*us_ctx;
	SSL		*us_ssl;
};

struct ustrdtab {
	int		 (*ust_init)(struct ustream *);
	int		 (*ust_close)(const struct ustream *);
	ssize_t		 (*ust_write)(const struct ustream *, const void *, size_t);
	char		*(*ust_gets)(struct ustream *, char *, int);
	int		 (*ust_error)(const struct ustream *);
	int		 (*ust_eof)(const struct ustream *);
};

struct ustream		*us_init(int, int, const char *);
int			 us_close(struct ustream *);
ssize_t			 us_write(const struct ustream *, const void *, size_t);
char			*us_gets(struct ustream *, char *, int);
int			 us_error(const struct ustream *);
int			 us_eof(const struct ustream *);

extern struct ustrdtab	*ustrdtabs[NUST];
