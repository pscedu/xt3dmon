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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 24242
#define MSGSIZ 1024

int
main(int argc, char *argv[])
{
	struct sockaddr_in sin;
	char buf[BUFSIZ];
	socklen_t salen;
	ssize_t len;
	int s;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket");
	memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SA_LEN
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr) != 1)
		err(1, "inet_pton");

	salen = sizeof(sin);
	if (connect(s, (struct sockaddr *)&sin, salen) == -1)
		err(1, "connect");
	fprintf(stderr, "Connected, sending command\n");
	snprintf(buf, sizeof(buf),
	    "sw: %d\n"
	    "sh: %d\n"
	    "x: %.3f\n"
	    "y: %.3f\n"
	    "z: %.3f\n"
	    "lx: %.3f\n"
	    "ly: %.3f\n"
	    "lz: %.3f\n"
	    "sid: clientsnapxx\n",
	    800, 450,
	    -4.0f, 32.8f, 51.5f,
	    0.6f, -0.40, -0.7);
	len = strlen(buf);
	if (write(s, buf, len) != len)
		err(1, "write");
	if (shutdown(s, SHUT_WR) == -1)
		err(1, "shutdown");
	fprintf(stderr, "Command sent, awaiting reply\n");
	if (read(s, buf, sizeof(buf)) != sizeof(buf))
		errx(1, "invalid server response");
	/* response is unused */
	while ((len = read(s, buf, sizeof(buf))) != -1 && len != 0)
		if (write(STDOUT_FILENO, buf, len) != len)
			err(1, "write");
	fprintf(stderr, "Done (%d)\n", len);
	if (len == -1)
		err(1, "read");
	close(s);
	exit(0);
}
