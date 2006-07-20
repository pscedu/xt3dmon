/* $Id$ */

#include "mon.h"

#include <stdio.h>
#include <stdlib.h>

#include "buf.h"

int
main(int argc, char *argv[])
{
	struct buf nbuf;
	char buf[BUFSIZ] = "some text that is     totally awesome and one "
	    "with the          powers ofthemighty nubus anddos command"
	    "interpreters running off of video cards.  45,56,243,45,56,23";

	text_wrap(&nbuf, buf, 20, "<\n  ", 2);
	printf("%s\n", buf_get(&nbuf));
	buf_free(&nbuf);


	buf_init(&nbuf);
	buf_appendfv(&nbuf, "foo %d", 164);
	buf_appendfv(&nbuf, "foo %d asdas ", 184);
	buf_append(&nbuf, '\0');
	printf("%s\n", buf_get(&nbuf));
	exit(0);
}
