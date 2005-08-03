/* $Id$ */

int
ffs(int i)
{
	int shift;

	for (shift = 0; shift < sizeof(int) * 8; shift++)
		if (i & (1 << shift))
			return (shift + 1);
	return (0);
}
