/* $Id$ */

#include <sys/socket.h>
#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "mon.h"

#define PORT	24242
#define Q	100
#define USLEEP	100

void serv_drawh(void);
int serv_parse(char *, struct state *, struct state *);;

int svc_x(char *, int *, struct state *, struct state *);
int svc_y(char *, int *, struct state *, struct state *);
int svc_z(char *, int *, struct state *, struct state *);
int svc_lx(char *, int *, struct state *, struct state *);
int svc_ly(char *, int *, struct state *, struct state *);
int svc_lz(char *, int *, struct state *, struct state *);

struct sv_cmd {
	const char	 *svc_name;
	int		(*svc_act)(char *, int *, struct state *, struct state *);
} sv_cmds[] = {
	{ "x",	svc_x },
	{ "y",	svc_y },
	{ "z",	svc_z },
	{ "lx",	svc_lx },
	{ "ly",	svc_ly },
	{ "lz",	svc_lz }
};

int sock;

int
svc_cmp(const void *a, const void *b)
{
	return (strcmp(((const struct sv_cmd *)a)->svc_name,
	    ((const struct sv_cmd *)b)->svc_name));
}

int
svc_findcmp(const void *k, const void *elem)
{
	const char *field = ((const struct sv_cmd *)elem)->svc_name;

	return (strncmp((const char *)k, field, strlen(field)));
}

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

	memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SA_LEN
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sz = sizeof(sin);
	if (bind(sock, (struct sockaddr *)&sin, sz) == -1)
		err(1, "bind");
	if (listen(sock, Q) == -1)
		err(1, "listen");

	st.st_opts &= ~(OP_DISPLAY | OP_TWEEN);

	drawh = serv_drawh;

	qsort(sv_cmds, sizeof(sv_cmds) / sizeof(sv_cmds[0]),
	    sizeof(sv_cmds[0]), svc_cmp);
}

#define SERVP_DONE	0
#define SERVP_CONT	1
#define SERVP_ERR	2

#define MAXTRIES 10
#define TRYWAIT 10	/* microseconds */

void
serv_drawh(void)
{
	struct sockaddr_in sin;
	struct state nst, stmask;
	char buf[BUFSIZ];
	socklen_t sz;
	int i, clifd;
	ssize_t len;

	sz = 0;
	nst = st;
	memset(&stmask, 0, sizeof(stmask));
	if ((clifd = accept(sock, (struct sockaddr *)&sin, &sz)) == -1) {
		if (errno != EWOULDBLOCK &&
		    errno != EAGAIN &&
		    errno != EINTR)
			err(1, "accept");
		usleep(USLEEP);
		return;
	}
//	sn_clear();
//	hl_restoreall();
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
		switch (serv_parse(buf, &nst, &stmask)) {
		case SERVP_ERR:
			goto drop;
		case SERVP_DONE:
			goto snap;
		}
	}
	if (i == MAXTRIES)
		goto drop;
snap:
	drawh_default();
	capture_snapfd(clifd, CM_PNG);
drop:
	close(clifd);
}

int
serv_parse(char *s, struct state *stp, struct state *mask)
{
	struct sv_cmd *svc;
	char *t;
	int len;

	for (t = s; *t != '\0'; t++) {
		while (isspace(*t))
			t++;
		svc = bsearch(t, sv_cmds, sizeof(sv_cmds) / sizeof(sv_cmds[0]),
		   sizeof(sv_cmds[0]), svc_findcmp);
		if (svc == NULL)
			return (SERVP_ERR);
		t += strlen(svc->svc_name);
		while (isspace(*t))
			t++;
		if (*t++ != ':')
			return (SERVP_ERR);
		while (isspace(*t))
			t++;
		if (!svc->svc_act(t, &len, stp, mask))
			return (SERVP_ERR);
		t += len;
		if (*t++ != '\n')
			return (SERVP_ERR);
	}
	if (mask->st_v.fv_x && mask->st_v.fv_y && mask->st_z &&
	    mask->st_lv.fv_x && mask->st_lv.fv_y && mask->st_lz)
		return (SERVP_DONE);
	else
		return (SERVP_CONT);
}

int
svc_x(char *t, int *used, struct state *stp, struct state *mask)
{
	if (mask->st_v.fv_x)
		return (0);
	if (sscanf(t, "%f%n", &stp->st_v.fv_x, used) != 1)
		return (0);
	mask->st_v.fv_x = 1.0f;
	return (1);
}

int
svc_y(char *t, int *used, struct state *stp, struct state *mask)
{
	if (mask->st_v.fv_y)
		return (0);
	if (sscanf(t, "%f%n", &stp->st_v.fv_y, used) != 1)
		return (0);
	mask->st_v.fv_y = 1.0f;
	return (1);
}

int
svc_z(char *t, int *used, struct state *stp, struct state *mask)
{
	if (mask->st_v.fv_z)
		return (0);
	if (sscanf(t, "%f%n", &stp->st_v.fv_z, used) != 1)
		return (0);
	mask->st_v.fv_z = 1.0f;
	return (1);
}

int
svc_lx(char *t, int *used, struct state *stp, struct state *mask)
{
	if (mask->st_lv.fv_x)
		return (0);
	if (sscanf(t, "%f%n", &stp->st_lv.fv_x, used) != 1)
		return (0);
	mask->st_lv.fv_x = 1.0f;
	return (1);
}

int
svc_ly(char *t, int *used, struct state *stp, struct state *mask)
{
	if (mask->st_lv.fv_y)
		return (0);
	if (sscanf(t, "%f%n", &stp->st_lv.fv_y, used) != 1)
		return (0);
	mask->st_lv.fv_y = 1.0f;
	return (1);
}

int
svc_lz(char *t, int *used, struct state *stp, struct state *mask)
{
	if (mask->st_lv.fv_z)
		return (0);
	if (sscanf(t, "%f%n", &stp->st_lv.fv_z, used) != 1)
		return (0);
	mask->st_lv.fv_z = 1.0f;
	return (1);
}
