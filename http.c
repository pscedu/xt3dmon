/* $Id$ */

typedef u_int16_t port_t;

struct http_req {
	const char	*htreq_server;
	port_t		 htreq_port;		/* host-byte order. */
	int		 htreq_flags;

	const char	*htreq_method;
	const char	*htreq_version;
	const char	*htreq_url;
	const char	**htreq_extra;		/* NULL-terminate. */
};

struct http_res {
};

#define HTF_SEND_HOST	(1<<0)

int
http_open(struct http_req *req, struct http_res *res)
{
	struct addrinfo hints, *ai, *res0;
	char *sport, *cause, buf[BUFSIZ];
	int needed, error, s;

	want = snprintf(NULL, 0, "%d", req->htreq_port);
	if (want == -1)
		err(1, "snprintf");
	want++;
	if ((sport = malloc(want)) == NULL)
		err(1, "malloc");
	snprintf(sport, want, "%d", req->htreq_port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((error = getaddrinfo(req->htreq_server, sport,
	    &hints, &res0)) != 0)
		err(1, "getaddrinfo");
	free(sport);

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

	snprintf(buf, "%s %s %s\r\n", req->htreq_method,
	    req->htreq_url, req->htreq_version);
	if (write(s, buf, strlen(buf)) != strlen(buf))
		err(1, "write");
	if (req->htreq_flags & HTF_SEND_HOST) {
		snprintf(buf, "Host: %s\r\n", req->htreq_server);
		if (write(s, buf, strlen(buf)) != strlen(buf))
			err(1, "write");
	}
	for (hdr = req->htreq_extra; hdr != NULL; hdr++)
		if (write(s, buf, strlen(buf)) != strlen(buf))
			err(1, "write");

	return (s);
}
