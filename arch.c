/* $Id$ */

#include "mon.h"

#include <sys/mman.h>

#include "uinp.h"

void
arch_init(void)
{
	/* Don't want passwords to swap to disk... */
	mlock(&login_auth, sizeof(login_auth));
	mlock(&authbuf, sizeof(authbuf));
}
