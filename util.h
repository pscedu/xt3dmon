/* $Id$ */

#include <sys/types.h>

#define CMP(a, b) \
	((a) < (b) ? -1 : ((a) == (b) ? 0 : 1))

#define SWAP(a, b, t)		\
	do {			\
		t = a;		\
		a = b;		\
		b = t;		\
	} while (0)

int		 baseconv(int);
void		 base64_encode(const void *, char *, size_t);
char		*strnchr(const char *, char, size_t);
const char	*smart_dirname(const char *);
const char	*smart_basename(const char *);
void		 escape_printf(struct buf *, const char *);
