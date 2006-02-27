/* $Id$ */

#include "util.h"

int
main(int argc, char *argv[])
{
	char buf[] = "asdf:asdf";
	char enc[3 * sizeof(buf)];

	base64_encode(buf, enc);
	printf("%s\n", buf);
	exit(0);
}
