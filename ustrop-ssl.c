/* $Id$ */

#include "compat.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "ustream.h"

#include <openssl/ssl.h>

struct ustrop ustrdtab_winsock[] = {
	ustrop_ssl_init,
	ustrop_ssl_close,
	ustrop_ssl_write,
	ustrop_ssl_gets,
	ustrop_ssl_error,
	ustrop_ssl_eof
};

	SSL_CTX *ctx;
	SSL *ssl;
	long l;

	port = PORT;
	progname = argv[0];
	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	host = NULL; /* gcc */
	switch (argc) {
	case 2:
		l = strtol(argv[1], NULL, 10);
		if (l > PORT_MAX || l < 1)
			errx(1, "invalid port");
		port = (int)l;
		/* FALLTHROUGH */
	case 1:
		host = argv[0];
		break;
	default:
		usage();
		/* NOTREACHED */
	}

	snprintf(portbuf, sizeof(portbuf), "%d", port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((error = getaddrinfo(host, portbuf, &hints, &res0)))
		errx(1, "%s", gai_strerror(error));

	s = -1;
	cause = NULL;
	for (res = res0; res; res = res->ai_next) {
		if ((s = socket(res->ai_family, res->ai_socktype,
		    res->ai_protocol)) == -1) {
			cause = "socket";
			continue;
		}
		if (connect(s, res->ai_addr, res->ai_addrlen) == -1) {
			cause = "connect";
			close(s);
			s = -1;
			continue;
		}
		if (res->ai_family == AF_INET)
			if (inet_ntop(res->ai_family,
			    &((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr,
			    buf, sizeof(buf)) != NULL)
			warnx("connected to %s", buf);
		break;
	}
	if (s == -1)
		err(1, "%s", cause);
	freeaddrinfo(res0);

	ctx = ctx_init(SSL_METH_CLI);
	ssl = SSL_new(ctx);
	SSL_set_fd(ssl, s);
	if (SSL_connect(ssl) != 1)
		errx(1, "%s", ssl_error());
	handle_conn(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	close(s);
	exit(0);
}

void
handle_conn(SSL *ssl)
{
	char *p, *t, buf[MSGLEN];
	struct cmd *cmd;
	ssize_t n;

	printf("login: ");
	fflush(stdout);

	if (fgets(buf, sizeof(buf), stdin) == NULL)
		errx(1, "fgets");
	if ((p = strchr(buf, '\n')) != NULL)
		*p = '\0';

	if (strlen(buf) == 0)
		errx(1, "no username entered");
	if (SSL_write(ssl, buf, sizeof(buf)) != sizeof(buf))
		errx(1, "write: %s", ssl_error());

	printf("password: ");
	fflush(stdout);

	disable_echo();
	if (fgets(buf, sizeof(buf), stdin) == NULL)
		errx(1, "fgets");
	enable_echo();
	printf("\n");

	if ((p = strchr(buf, '\n')) != NULL)
		*p = '\0';

	if (strlen(buf) == 0)
		errx(1, "no password entered");
	if (SSL_write(ssl, buf, sizeof(buf)) != sizeof(buf))
		errx(1, "write: %s", ssl_error());

	if ((n = SSL_read(ssl, buf, sizeof(buf))) == -1)
		errx(1, "read: %s", ssl_error());
	if (n == 0) {
		warnx("unexpected EOF from server");
		return;
	}
	printf("server> %s", buf);
	if (buf[strlen(buf) - 1] != '\n')
		printf("\n");
	if (strncmp(buf, MSG_SUCCESS, strlen(MSG_SUCCESS)) != 0)
		return;

	do {
		if ((n = SSL_read(ssl, buf, sizeof(buf))) == -1)
			errx(1, "read: %s", ssl_error());
		if (n == 0) {
			warnx("unexpected EOF from server");
			return;
		}
		if ((t = strstr(buf, MSG_READY)) != NULL) {
			*t = '\0';
			n = strlen(buf);
		}
		printf("server> %*s", n, buf);
	} while (t == NULL);

	for (;;) {
		printf("myfs> ");
		fflush(stdout);
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		t = buf;
		while (isspace(*t))
			t++;
		if (*t == '\0')
			continue;
		if ((p = strchr(t, '\n')) != NULL)
			*p = '\0';
		if ((p = strpbrk(t, " ")) != NULL) {
			*p++ = '\0';
			while (isspace(*p))
				p++;
		}
		if ((cmd = find_cmd(cmds, t)) == NULL) {
			warnx("unrecognized client command: %s", buf);
			continue;
		}
		cmd->c_handler(ssl, p);
	}
	if (ferror(stdin))
		err(1, "fgets");
	printf("\n");
	cmd_quit(ssl, NULL);
}

struct cmd *
find_cmd(struct cmd *cmds, const char *cmd)
{
	struct cmd *c;

	for (c = cmds; c->c_name != NULL; c++)
		if (strcmp(c->c_name, cmd) == 0)
			return (c);
	return (NULL);
}

void
cmd_get(SSL *ssl, const char *file)
{
	char *p, *t, *msg, buf[MSGLEN];
	int remaining, chunksiz;
	ssize_t n;
	FILE *fp;
	long l;

	if (file[0] == '\0') {
		warn("get: invalid filename");
		return;
	}

	if ((fp = fopen(file, "wb")) == NULL) {
		warn("%s", file);
		return;
	}

	snprintf(buf, sizeof(buf), "get %s", file);
	if (SSL_write(ssl, buf, sizeof(buf)) != (ssize_t)sizeof(buf))
		errx(1, "write: %s", ssl_error());
	/* grab remote filesize */
	if ((n = SSL_read(ssl, buf, sizeof(buf))) == -1)
		errx(1, "read: %s", ssl_error());
	if (n == 0)
		goto done;
	msg = "error: ";
	if (strncmp(buf, msg, strlen(msg)) == 0) {
		printf("server> %s: %*s", file, n - 1 - strlen(msg),
		    buf + strlen(msg));
		goto done;
	}
	msg = "size: ";
	if (strncmp(buf, msg, strlen(msg)) != 0) {
		warnx("received malformed response from server: %*s", n, buf);
		goto done;
	}
	t = p = buf + strlen(msg);
	while (isdigit(*p))
		p++;
	*p = '\0';
	if (t == p) {
		warnx("received malformed response from server");
		goto done;
	}
	l = strtol(t, NULL, 10);
	remaining = (int)l;

	while (remaining > 0) {
		if (remaining > (int)sizeof(buf))
			chunksiz = remaining;
		else
			chunksiz = sizeof(buf);
		if ((n = SSL_read(ssl, buf, chunksiz)) == -1)
			errx(1, "read: %s", ssl_error());
		else if (n == 0) {
			warnx("received unexpected EOF from server");
			goto done;
		}
		if (fwrite(buf, 1, n, fp) != (size_t)n)
			err(1, "fwrite");
		remaining -= n;
	}
done:
	fclose(fp);
}

void
cmd_put(SSL *ssl, const char *file)
{
	struct stat st;
	char buf[MSGLEN];
	ssize_t n;
	FILE *fp;

	if (file[0] == '\0') {
		warnx("put: invalid file");
		return;
	}

	if ((fp = fopen(file, "rb")) == NULL ||
	    fstat(fileno(fp), &st) == -1) {
		warn("put: %s", file);
		return;
	}

	snprintf(buf, sizeof(buf), "put %s\n", file);
	if (SSL_write(ssl, buf, sizeof(buf)) != (ssize_t)sizeof(buf))
		errx(1, "write: %s", ssl_error());

	snprintf(buf, sizeof(buf), "size: %ld\n", (long)st.st_size);
	if (SSL_write(ssl, buf, sizeof(buf)) != (ssize_t)sizeof(buf))
		errx(1, "write: %s", ssl_error());

	while ((n = fread(buf, 1, sizeof(buf), fp)) != 0) {
		if (SSL_write(ssl, buf, n) != n) {
			warnx("write: %s", ssl_error());
			break;
		}
	}
	if (ferror(fp))
		warn("fread");
	fclose(fp);
}

void
cmd_quit(SSL *ssl, __unused const char *p)
{
	char buf[MSGLEN];

	snprintf(buf, sizeof(buf), "quit\n");
	if (SSL_write(ssl, buf, sizeof(buf)) != (ssize_t)sizeof(buf))
		errx(1, "write: %s", ssl_error());
	exit(0);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s hostname [port]\n", progname);
	exit(1);
}
