/* $Id$ */

#include "mon.h"

__inline int
negmod(int a, int b)
{
	int c;

	if (a < 0) {
		c = -a;
		c %= b;
		if (c)
			c = b - c;
	} else
		c = a % b;
	return (c);
}
