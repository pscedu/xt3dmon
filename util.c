/* $Id$ */

#include "mon.h"

#include <libgen.h>
#include <stdio.h>

#include "util.h"

/* Special case of base 2 to base 10. */
int
baseconv(int n)
{
	unsigned int i;

	for (i = 0; i < sizeof(n) * 8; i++)
		if (n & (1 << i))
			return (i + 1);
	return (0);
}

/*
 * enc buffer must be 4/3+1 the size of buf.
 * Note: enc and buf are NOT C-strings.
 */
void
base64_encode(const void *buf, char *enc, size_t siz)
{
	static char pres[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz0123456789+/";
	const unsigned char *p;
	u_int32_t val;
	size_t pos;
	int i;

	i = 0;
	for (pos = 0, p = buf; pos < siz; pos += 3, p += 3) {
		/*
		 * Convert 3 bytes of input (3*8 bits) into
		 * 4 bytes of output (4*6 bits).
		 *
		 * If fewer than 3 bytes are available for this
		 * round, use zeroes in their place.
		 */
		val = p[0] << 16;
		if (pos + 1 < siz)
			val |= p[1] << 8;
		if (pos + 2 < siz)
			val |= p[2];

		enc[i++] = pres[val >> 18];
		enc[i++] = pres[(val >> 12) & 0x3f];
		if (pos + 1 >= siz)
			break;
		enc[i++] = pres[(val >> 6) & 0x3f];
		if (pos + 2 >= siz)
			break;
		enc[i++] = pres[val & 0x3f];
	}
	if (pos + 1 >= siz) {
		enc[i++] = '=';
		enc[i++] = '=';
	} else if (pos + 2 >= siz)
		enc[i++] = '=';
	enc[i++] = '\0';
printf("base64: wrote %d chars\n", i);
}

/* Like strchr, but bound before NUL. */
char *
strnchr(const char *s, char c, size_t len)
{
	size_t pos;

	for (pos = 0; s[pos] != c && pos < len; pos++)
		;

	if (pos == len)
		return (NULL);
	return ((char *)(s + pos));
}

const char *
smart_basename(const char *fn)
{
	static char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s", fn);
	return (basename(path));
}

const char *
smart_dirname(const char *fn)
{
	static char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s", fn);
	return (dirname(path));
}
