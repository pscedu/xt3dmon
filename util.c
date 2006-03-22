/* $Id$ */

#include "mon.h"

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

/* enc buffer must be (strlen(buf) * 3 + 1). */
void
base64_encode(const char *buf, char *enc)
{
	static char pres[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz0123456789+/";
	const char *p;
	u_int32_t val;
	int i;

	i = 0;
	for (p = buf; *p != '\0'; p += 3) {
		/*
		 * Convert 3 bytes of input (3*8 bits) into
		 * 4 bytes of output (4*6 bits).
		 *
		 * If fewer than 3 bytes are available for this
		 * round, use zeroes in their place.
		 */
		val = p[0] << 16;
		if (p[1] != '\0')
			val |= p[1] << 8;
		if (p[2] != '\0')
			val |= p[2];

		enc[i++] = pres[val >> 18];
		enc[i++] = pres[(val >> 12) & 0x3f];
		if (p[1] == '\0')
				break;
		enc[i++] = pres[(val >> 6) & 0x3f];
		if (p[2] == '\0')
			break;
		enc[i++] = pres[val & 0x3f];
	}
	if (p[1] == '\0') {
		enc[i++] = '=';
		enc[i++] = '=';
	} else if (p[2] == '\0')
		enc[i++] = '=';
	enc[i] = '\0';
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
