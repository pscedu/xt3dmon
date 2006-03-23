/* $Id$ */

#include "mon.h"

#include "ustream.h"

extern struct ustrdtab ustrdtab_file;
extern struct ustrdtab ustrdtab_ssl;

struct ustrdtab *ustrdtabs[] = {
	&ustrdtab_file,	/* LOCAL */
	&ustrdtab_file,	/* REMOTE */
	&ustrdtab_ssl	/* SSL */
};
