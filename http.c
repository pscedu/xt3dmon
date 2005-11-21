/* $Id$ */

#include "compat.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "cdefs.h"
#include "mon.h"

int
net_connect(const char *host, port_t port)
{
	struct addrinfo hints, *ai, *res0;
	int want, error, s;
	char *sport, *cause;

	want = snprintf(NULL, 0, "%d", port);
	if (want == -1)
		err(1, "snprintf");
	want++;
	if ((sport = malloc(want)) == NULL)
		err(1, "malloc");
	snprintf(sport, want, "%d", port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((error = getaddrinfo(host, sport, &hints, &res0)) != 0)
		err(1, "getaddrinfo %s", host);
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

int
http_open(struct http_req *req, __unused struct http_res *res)
{
	const char **hdr, *p;
	char buf[BUFSIZ];
	int s, sdup;
	FILE *fp;

	s = net_connect(req->htreq_server, req->htreq_port);
	snprintf(buf, sizeof(buf), "%s %s %s\r\n", req->htreq_method,
	    req->htreq_url, req->htreq_version);
	if (write(s, buf, strlen(buf)) != (int)strlen(buf))
		err(1, "write");
	/* XXX: disgusting */
	if (strcmp(req->htreq_version, "HTTP/1.1") == 0) {
		snprintf(buf, sizeof(buf), "Host: %s\r\nConnection: close\r\n",
		    req->htreq_server);
		if (write(s, buf, strlen(buf)) != (int)strlen(buf))
			err(1, "write");
	}
	for (hdr = req->htreq_extra; hdr != NULL; hdr++)
		if (write(s, *hdr, strlen(*hdr)) != (int)strlen(*hdr))
			err(1, "write");
	snprintf(buf, sizeof(buf), "\r\n");
	if (write(s, buf, strlen(buf)) != (int)strlen(buf))
		err(1, "write");
	if ((sdup = dup(s)) == -1)
		err(1, "dup");
	if ((fp = fdopen(sdup, "r")) == NULL)
		err(1, "fdopen");
	setbuf(fp, NULL);
	/* XXX: check status */
	while (fgets(buf, sizeof(buf), fp) != NULL)
		if (strcmp(buf, "\r\n") == 0)
			break;
	if (ferror(fp))
		err(1, "fgets");
	fclose(fp);
	errno = 0;
	return (s);
}
