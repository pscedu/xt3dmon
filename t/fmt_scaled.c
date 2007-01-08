/* $Id$ */

#include <stdio.h>

#define TEST(val)						\
	do {							\
		fmt_scaled((val), buf);				\
		printf("%s\t(%d):\t%s\n", #val, val, buf);	\
	} while(0)

int
main(int argc, char *argv[])
{
	char buf[50];

	TEST(1024 * 1024 * 5 + 50000);
	TEST(1);
	TEST(1023);
	TEST(1024);
	TEST(1024 * 1024);
	TEST(1024 * 1010);
	TEST(1024 * 1024 - 1);
	TEST(1024 * 1024 + 1);
	exit(0);
}
