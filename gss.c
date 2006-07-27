/* $Id$ */

#include "mon.h"

#include <err.h>
#include <string.h>

#include <krb5.h>

#ifdef HEIMDAL
# include <gssapi.h>
#else
# include <gssapi/gssapi.h>
# include <gssapi/gssapi_generic.h>
# include <gssapi/gssapi_krb5.h>
#endif

#include "ds.h"
#include "ustream.h"
#include "util.h"

gss_name_t	 gss_server;
gss_ctx_id_t	 gss_ctx;
OM_uint32	 gss_minor;
gss_buffer_desc	 gss_otoken = GSS_C_EMPTY_BUFFER;

void
gss_finish(void)
{
	gss_release_name(&gss_minor, &gss_server);
	if (gss_ctx != GSS_C_NO_CONTEXT)
		gss_delete_sec_context(&gss_minor,
		    &gss_ctx, GSS_C_NO_BUFFER);
}

void
gss_build_auth(const struct ustream *us)
{
	const char authline[] = "Authorization: Negotiate ";
	const char nl[] = "\r\n";
	size_t bsiz;
	char *p;

	bsiz = (gss_otoken.length + 3) * 4 / 3 + 1;
	if ((p = malloc(bsiz)) == NULL)
		err(1, "malloc");
	base64_encode(gss_otoken.value, p, gss_otoken.length);

	if (us_write(us, authline, strlen(authline)) != strlen(authline))
		err(1, "us_write");

	if (us_write(us, p, bsiz - 1) != (ssize_t)(bsiz - 1))
		err(1, "us_write");
	free(p);

	if (us_write(us, nl, strlen(nl)) != strlen(nl))
		err(1, "us_write");

	gss_finish();
}

int
gss_valid(const char *host)
{
	gss_OID_desc krb5_oid = {9, "\x2a\x86\x48\x86\xf7\x12\x01\x02\x02"};
	gss_buffer_desc itoken = GSS_C_EMPTY_BUFFER;
	OM_uint32 major, rflags, rtime;
	const char service[] = "HTTP";
	gss_OID oid;

	/* Default to MIT Kerberos */
	oid = &krb5_oid;

#if 0
	/* XXX - Determine the underlying authentication mechanisms and check for SPNEGO */
	major = gss_indiate_mechs(&gss_minor, &mset);
	if (GSS_ERROR(major))
		errx(1, "gss_indiate_mechs failed");
	/* XXX - SPNEGO - for windows */
#endif

	/* itoken = "HTTP/f.q.d.n" aka "HTTP@hostname.foo.bar" */
	if ((itoken.length = asprintf((char **)&itoken.value,
	    "%s@%s", service, host)) == (size_t)-1)
		err(1, "asprintf");

	/* Convert the printable name to an internal format */
	major = gss_import_name(&gss_minor, &itoken,
	    GSS_C_NT_HOSTBASED_SERVICE, &gss_server);

	free(itoken.value);

	if (GSS_ERROR(major))
		errx(1, "gss_import_name");

	/* Initiate a security context */
	rflags = 0;
	rtime = GSS_C_INDEFINITE;
	gss_ctx = GSS_C_NO_CONTEXT;
	major = gss_init_sec_context(&gss_minor, GSS_C_NO_CREDENTIAL,
	    &gss_ctx, gss_server, oid, rflags, rtime,
	    GSS_C_NO_CHANNEL_BINDINGS, GSS_C_NO_BUFFER,
	    NULL, &gss_otoken, NULL, NULL);

	if (GSS_ERROR(major) || gss_otoken.length == 0) {
		gss_finish();
		warnx("gss_init_sec_context");
		return (0);
	}
	return (1);
}