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

#include "mon.h"

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "cdefs.h"
#include "http.h"
#include "ustream.h"

int
net_connect(const char *host, const char *port)
{
	struct addrinfo hints, *ai, *res0;
	int error, s;
	char *cause;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((error = getaddrinfo(host, port, &hints, &res0)) != 0)
		errx(1, "getaddrinfo %s: %s", host, gai_strerror(error));

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
		err(1, "%s: %s:%s", cause, host, port);
	freeaddrinfo(res0);
	return (s);
}

struct ustream *
http_open(struct http_req *req, struct http_res *res)
{
	char *s, *p, **hdr, buf[BUFSIZ];
	int flags, fd, ust;
	struct ustream *us;
	size_t len;
	long l;

	fd = net_connect(req->htreq_server, req->htreq_port);
	if (fd == -1)
		return (NULL);

	/* XXX make this better */
	ust = UST_REMOTE;
	if (strcmp(req->htreq_port, "https") == 0 ||
	    strcmp(req->htreq_port, "443") == 0)
		ust = UST_SSL;

	if ((us = us_init(fd, ust, "rw")) == NULL)
		return (NULL);

	snprintf(buf, sizeof(buf), "%s %s %s\r\n", req->htreq_method,
	    req->htreq_url, req->htreq_version);
	if (us_write(us, buf, strlen(buf)) != (ssize_t)strlen(buf))
		err(1, "us_write");
	/* XXX: disgusting */
	if (strcmp(req->htreq_version, "HTTP/1.1") == 0) {
		snprintf(buf, sizeof(buf), "Host: %s\r\nConnection: close\r\n",
		    req->htreq_server);
		if (us_write(us, buf, strlen(buf)) != (ssize_t)strlen(buf))
			err(1, "us_write");
	}
	if (req->htreq_extra != NULL)
		for (hdr = req->htreq_extra; *hdr != NULL; hdr++)
			if (us_write(us, *hdr, strlen(*hdr)) !=
			    (ssize_t)strlen(*hdr))
				err(1, "us_write");
	if (req->htreq_extraf != NULL)
		req->htreq_extraf(us);
	snprintf(buf, sizeof(buf), "\r\n");
	if (us_write(us, buf, strlen(buf)) != (ssize_t)strlen(buf))
		err(1, "us_write");

	flags = 0;
	while (us_gets(us, buf, sizeof(buf)) != NULL) {
		if (strcmp(buf, "\r\n") == 0)
			break;

		s = "HTTP/1.1 ";
		len = strlen(s);
		if (strncmp(buf, s, len) == 0) {
			s = p = buf + len;
			while (isdigit(*p))
				p++;
			*p++ = '\0';

			l = strtoul(s, NULL, 10);
			if (l >= 0 && l <= INT_MAX)
				res->htres_code = (int)l;
		}

		s = "Last-Modified: ";
		len = strlen(s);
		if (strncmp(buf, s, len) == 0) {
			time_t t, lt, gt;
			struct tm tm;
			char *endp;

			lt = time(NULL);
			gmtime_r(&lt, &tm);
			gt = mktime(&tm);

			endp = strptime(buf + len, "%a, %d %h %Y %T", &tm);
			if (endp != NULL &&
			    strncmp(endp, " GMT", 4) == 0) {
				tm.tm_isdst = 0;
				t = mktime(&tm);
				t += lt - gt;
				localtime_r(&t, &res->htres_mtime);
			}
		}

		s = "Transfer-Encoding: chunked";
		len = strlen(s);
		if (strncmp(buf, s, len) == 0)
			flags |= USF_HTTP_CHUNK;
	}
	if (us_sawerror(us))
		errx(1, "us_gets: %s", us_errstr(us));
	us->us_flags = flags;
	errno = 0;
	return (us);
}
