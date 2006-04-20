/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>

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

__inline double
negmodf(double a, double b)
{
	double c;

	if (a < 0) {
		c = -a;
		c = fmod(c, b);
		if (c)
			c = b - c;
	} else
		c = fmod(a, b);
	return (c);
}
