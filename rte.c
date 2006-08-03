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
