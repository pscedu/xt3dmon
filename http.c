/* $Id$ */

#include "compat.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "cdefs.h"
#include "mon.h"
#include "ustream.h"

int
net_connect(const char *host, in_port_t port)
{
	struct addrinfo hints, *ai, *res0;
	int error, s;
	char *sport, *cause;

	if (asprintf(&sport, "%d", port) == -1)
		err(1, "asprintf");

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((error = getaddrinfo(host, sport, &hints, &res0)) != 0)
		errx(1, "getaddrinfo %s: %s", host, gai_strerror(error));
	free(sport);

	s = -1;
	cause = NULL; /* gcc */
	for (ai = res0; ai != NULL; ai = ai->ai_next) {
		if ((s = socket(ai->ai_family, ai->ai_socktype,
		    ai->ai_protocol)) == -1) {
			cause = "socket";
			continue;
		}
		if (connect(s, ai->ai_addr, ai->ai_addrlen) == -1) {
			cause = "connect";
			close(s);
			s = -1;
			continue;
		}
		break;
	}
	if (s == -1)
		err(1, "%s", cause);
	freeaddrinfo(res0);
	return (s);
}

struct ustream *
http_open(struct http_req *req, __unused struct http_res *res)
{
	char **hdr, buf[BUFSIZ];
	struct ustream *us;
	int fd;

	fd = net_connect(req->htreq_server, req->htreq_port);
	if (fd == -1)
		return (NULL);
	us = us_init(fd, UST_REMOTE, "rw");

	snprintf(buf, sizeof(buf), "%s %s %s\r\n", req->htreq_method,
	    req->htreq_url, req->htreq_version);
	if (us_write(us, buf, strlen(buf)) != (int)strlen(buf))
		err(1, "us_write");
	/* XXX: disgusting */
	if (strcmp(req->htreq_version, "HTTP/1.1") == 0) {
		snprintf(buf, sizeof(buf), "Host: %s\r\nConnection: close\r\n",
		    req->htreq_server);
		if (us_write(us, buf, strlen(buf)) != (int)strlen(buf))
			err(1, "us_write");
	}
	if (req->htreq_extra != NULL)
		for (hdr = req->htreq_extra; *hdr != NULL; hdr++)
			if (us_write(us, *hdr, strlen(*hdr)) !=
			    (int)strlen(*hdr))
				err(1, "us_write");
	snprintf(buf, sizeof(buf), "\r\n");
	if (us_write(us, buf, strlen(buf)) != (int)strlen(buf))
		err(1, "us_write");

	/* XXX: check http status */
	while (us_gets(us, buf, sizeof(buf)) != NULL)
		if (strcmp(buf, "\r\n") == 0)
			break;
	if (us_error(us))
		err(1, "us_gets");
	errno = 0;
	return (us);
}
