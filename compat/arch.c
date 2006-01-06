/* $Id$ */

#include "compat.h"

#include <err.h>

void
arch_init(void)
{
	WSADATA wsa;
	int error;

	if ((error = WSAStartup(MAKEWORD(2, 2), &wsa)) != 0)
		errx(1, "WSAStartup: %d", error);
}
