/* $Id$ */

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "xssl.h"

const char *
ssl_error(void)
{
#define ERRBUF_LEN 120
        static char errbuf[ERRBUF_LEN];

        return (ERR_error_string(ERR_get_error(), errbuf));
}


void
ssl_init(void)
{
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
}
