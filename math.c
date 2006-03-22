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

void
matrix_multiply(double *r, double *a, size_t anc, size_t anr,
   double *b, size_t bnc, size_t bnr)
{
	size_t i, j, k, l;
	double val;

	if (anr != bnc)
		errx(1, "matrix_multiply: a_nrows != b_ncols");

	for (i = 0; i < anc; i++)
		for (j = 0; j < bnr; j++) {
			val = 0;
			for (k = 0; k < anr; k++)
				for (l = 0; l < bnc; l++)
					val += a[i * anr + k] * b[j * bnr + l];
			r[i * anr + j] = val;
		}
}
