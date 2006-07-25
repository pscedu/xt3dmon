/* $Id$ */

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
