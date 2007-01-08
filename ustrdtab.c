/* $Id$ */

/*
 * "Universal stream" dispatch table.
 * This file lists the correct handles for each stream type.
 */

#include "mon.h"

#include "ustream.h"

extern struct ustrdtab ustrdtab_file;
extern struct ustrdtab ustrdtab_ssl;
extern struct ustrdtab ustrdtab_gzip;

struct ustrdtab *ustrdtabs[] = {
	&ustrdtab_file,	/* LOCAL */
	&ustrdtab_file,	/* REMOTE */
	&ustrdtab_ssl,	/* SSL */
	&ustrdtab_gzip	/* GZIP */
};
