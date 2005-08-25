/* $Id$ */

#include <sys/socket.h>
#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "mon.h"

#define PORT	24242
#define Q	100
#define USLEEP	100

int sock;

void
serv_init(void)
{
	struct sockaddr_in sin;
	socklen_t sz;
	int fflags;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	if (fcntl(sock, F_GETFL, &fflags) == -1)
		err(1, "fcntl F_GETFL");
	fflags |= O_NONBLOCK;
	if (fcntl(sock, F_SETFL, &fflags) == -1)
		err(1, "fcntl F_SETFL");

#ifdef HAVE_SA_LEN
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *)&sin, sz) == -1)
		err(1, "bind");
	if (listen(sock, Q) == -1)
		err(1, "listen");

	st.st_opts &= ~(OP_DISPLAY | OP_TWEEN);
}

#define SERVP_DONE	0
#define SERVP_CONT	1
#define SERVP_ERR	2

int
serv_parse(char *s, struct state *stp)
{
	float *field;
	char where;
	char *t;
	int len;

	where = '\0';
	for (t = s; *t != '\0'; t++) {
		while (isspace(*t))
			t++;
		if (strcmp(t, "x"))
			t++, field = &stp->st_v.fv_x;
		else if (strcmp(t, "y"))
			t++, field = &stp->st_v.fv_y;
		else if (strcmp(t, "z"))
			t++, field = &stp->st_v.fv_z;
		else if (strcmp(t, "lx"))
			t += 2, field = &stp->st_lv.fv_x;
		else if (strcmp(t, "ly"))
			t += 2, field = &stp->st_lv.fv_y;
		else if (strcmp(t, "lz"))
			t += 2, field = &stp->st_lv.fv_z;
		else
			return (SERVP_ERR);
		while (isspace(*t))
			t++;
		if (*t++ != ':')
			return (SERVP_ERR);
		while (isspace(*t))
			t++;
		if (sscanf(t, "%f%n", field, &len) != 1)
			return (SERVP_ERR);
		t += len;
		if (*t++ != '\n')
			return (SERVP_ERR);
	}
	return (SERVP_CONT);
}

#define MAXTRIES 10
#define TRYWAIT 10	/* microseconds */

void
serv_drawh(void)
{
	struct sockaddr_in sin;
	struct state nst;
	char buf[BUFSIZ];
	socklen_t sz;
	int i, clifd;
	ssize_t len;

	sz = 0;
	memset(&nst, 0, sizeof(nst));
	if ((clifd = accept(sock, (struct sockaddr *)&sin, &sz)) == -1) {
		if (errno != EWOULDBLOCK &&
		    errno != EAGAIN &&
		    errno != EINTR)
			err(1, "accept");
		usleep(USLEEP);
		return;
	}
	for (i = 0; i < MAXTRIES; i++) {
		usleep(TRYWAIT);
		if ((len = read(clifd, buf, sizeof(buf) - 1)) == -1) {
			if (errno != EWOULDBLOCK &&
			    errno != EAGAIN &&
			    errno != EINTR)
				err(1, "read");
			continue;
		}
		buf[len] = '\0';
		switch (serv_parse(buf, &nst)) {
		case SERVP_ERR:
			goto drop;
		case SERVP_DONE:
			goto snap;
		}
	}
snap:
	drawh_default();
	capture_snapfd(clifd, CM_PNG);
drop:
	close(clifd);
}

