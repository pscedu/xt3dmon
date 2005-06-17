/* $Id$ */

#include <err.h>
#include <stdio.h>

#include <mysql.h>

#define DBH_HOST	"localhost"
#define DBH_USER	""
#define DBH_PASS	""
#define DBH_PORT	3307
#define DBH_SOCK	NULL
#define DBH_DB		"xtnodes"

struct dbh {
	union {
		MYSQL dbhu_mysql;
	} dbh_u;
#define dbh_mysql dbh_u.dbhu_mysql
};

struct dbh dbh;

void
db_connect(struct dbh *dbh)
{
	if (mysql_real_connect(&dbh->dbh_mysql, DBH_HOST,
	    DBH_USER, DBH_PASS, DBH_DB, DBH_PORT, DBH_SOCK, 0) == NULL)
		err(1, "%s", mysql_error(&dbh->dbh_mysql));
}

void
db_disconnect(struct dbh *dbh)
{
	mysql_close(&dbh->dbh_mysql);
}

void
db_jobmap(void)
{
}
