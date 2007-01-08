/* $Id$ */

/*
 * "Universal stream" dispatch table.
 * This file lists the correct handles for each stream type.
 * This file is necessay because Windows can't use system calls
 * on sockets, so a separate winsock file for socket operations
 * (read vs. recv, write vs. send) is provided.
 */

#include "ustream.h"

extern struct ustrdtab ustrdtab_file;
extern struct ustrdtab ustrdtab_winsock;
extern struct ustrdtab ustrdtab_ssl;
extern struct ustrdtab ustrdtab_gzip;

struct ustrdtab *ustrdtabs[] = {
	&ustrdtab_file,		/* LOCAL */
	&ustrdtab_winsock,	/* REMOTE */
	&ustrdtab_ssl,		/* SSL */
	&ustrdtab_gzip		/* GZIP */
};
