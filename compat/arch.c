/* $Id$ */

#include "compat.h"

void
arch_init(void)
{
	WSADATA wsa;
	int error;

	if ((error = WSAStartup(MAKEWORD(2, 2), &wsa)) != 0)
		errx("WSAStartup: %d", error);
}
