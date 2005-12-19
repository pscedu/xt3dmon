/* $Id$ */

#include <stdio.h>
#include "mon.h"

int
main(int argc, char *argv[])
{
	char buf[] = "some text that is totally awesome and one"
	    "with the powers of the mighty nubus and dos command"
	    "interpreters running off of video cards";

	text_wrap(buf, sizeof(buf), 20);
	printf("%s\n", buf);
	exit(0);
}
