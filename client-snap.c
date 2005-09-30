/* $Id$ */

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
	    "lz: %.3f\n",
	    800, 450,
	    -4.0f, 32.8f, 51.5f,
	    0.6f, -0.40, -0.7);
	len = strlen(buf);
	if (write(s, buf, len) != len)
		err(1, "write");
	if (shutdown(s, SHUT_WR) == -1)
		err(1, "shutdown");
	fprintf(stderr, "Command sent, awaiting reply\n");
	while ((len = read(s, buf, sizeof(buf))) != -1 && len != 0)
		if (write(STDOUT_FILENO, buf, len) != len)
			err(1, "write");
	fprintf(stderr, "Done (%d)\n", len);
	if (len == -1)
		err(1, "read");
	close(s);
	exit(0);
}
