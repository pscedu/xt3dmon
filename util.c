/* $Id$ */

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
	char *p, t[3];
	int i;
	uint32_t val;

	i = 0;
	for (p = buf; *p != '\0'; p += 3) {
		/*
		 * Convert 3 bytes of input (3*8 bits) into
		 * 4 bytes of output (4*6 bits).
		 */
		val = (p[0] << 16) | (p[1] << 8) | p[2];
		enc[i++] = pres[val >> 18];
		enc[i++] = pres[(val >> 12) & 0x3f];
		enc[i++] = pres[(val >> 6) & 0x3f];
		enc[i++] = pres[val & 0x3f];
	}
	for (; i / 3 != 0; i++)
		enc[i++] = '=';
	enc[i] = '\0';
}
