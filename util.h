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

int	 baseconv(int);
void	 base64_encode(const char *, char *, size_t);
char	*strnchr(const char *, char, size_t);
