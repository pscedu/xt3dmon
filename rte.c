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
 * Routing error utility routines.
 */

#include "mon.h"

#include <err.h>

#include "env.h"
#include "route.h"

int
rteport_to_rdir(int port)
{
	switch (port) {
	case RP_NEGX:
		return (RD_NEGX);
	case RP_POSX:
		return (RD_POSX);
	case RP_NEGY:
		return (RD_NEGY);
	case RP_POSY:
		return (RD_POSY);
	case RP_NEGZ:
		return (RD_NEGZ);
	case RP_POSZ:
		return (RD_POSZ);
	default:
		err(1, "invalid rteport: %d", port);
	}
}

int
rdir_to_rteport(int rd)
{
	switch (rd) {
	case RD_NEGX:
		return (RP_NEGX);
	case RD_POSX:
		return (RP_POSX);
	case RD_NEGY:
		return (RP_NEGY);
	case RD_POSY:
		return (RP_POSY);
	case RD_NEGZ:
		return (RP_NEGZ);
	case RD_POSZ:
		return (RP_POSZ);
	default:
		err(1, "invalid rdir: %d", rd);
	}
}
