/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "mach.h"
#include "xmath.h"

int *
physcoord_new(void)
{
	int *pc;

	if ((pc = calloc(machine.m_nphysdim, sizeof(*pc))) == NULL)
		err(1, "calloc");
	return (pc);
}
