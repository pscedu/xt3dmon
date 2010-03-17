/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

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
