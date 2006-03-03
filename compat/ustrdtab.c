/* $Id$ */

#include "ustream.h"

extern struct ustrdtab ustrdtab_file;
extern struct ustrdtab ustrdtab_winsock;
extern struct ustrdtab ustrdtab_ssl;

struct ustrdtab *ustrdtabs[] = {
	&ustrdtab_file,		/* LOCAL */
	&ustrdtab_winsock,	/* REMOTE */
	&ustrdtab_ssl,		/* SSL */
};
