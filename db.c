/* $Id$ */

#include "compat.h"

#include <err.h>
#include <limits.h>
#include <stdio.h>

#include <mysql.h>

#include "mon.h"

#define DBH_HOST	"localhost"
#define DBH_USER	"basic"
#define DBH_PASS	"basic"
#define DBH_PORT	3306
#define DBH_SOCK	NULL
#define DBH_DB		"XTAdmin"

struct dbh dbh;

const char *
dbh_error(struct dbh *dbh)
{
	return (mysql_error(&dbh->dbh_mysql));
}

void
dbh_connect(struct dbh *dbh)
{
	if (mysql_real_connect(&dbh->dbh_mysql, DBH_HOST,
	    DBH_USER, DBH_PASS, DBH_DB, DBH_PORT, DBH_SOCK, 0) == NULL)
		err(1, "%s", dbh_error(dbh));
}

void
dbh_disconnect(struct dbh *dbh)
{
	mysql_close(&dbh->dbh_mysql);
}

struct db_map_ent {
	const char	*dme_name;
	int		 dme_code;
	int		 dme_doset;
} db_status_map[] = {
	{ "up",		JST_FREE,	0 },
	{ "down",	JST_DOWN,	1 },
	{ "unavail",	JST_DISABLED,	1 },
	{ "route",	JST_UNACC,	0 },
	{ "suspect",	JST_CHECK,	1 },
	{ "admindown",	JST_BAD,	0 },
	{ NULL,		0,		0 }
}, db_type_map[] = {
	{ "service",	JST_IO,		1 },
	{ "compute",	JST_FREE,	0 },
	{ NULL,		0,		0 }
};

void
db_map_set(struct db_map_ent *map, const char *dbval, int *field)
{
	struct db_map_ent *dme;

	for (dme = map; dme->dme_name != NULL; dme++)
		if (strcmp(dme->dme_name, dbval) == 0) {
			if (dme->dme_doset)
				*field = dme->dme_code;
			return;
		}
	warnx("%s: database value not found in map", dbval);
}

void
db_physmap(void)
{
	int x, y, z, nid, len, r, cb, cg, m, n;
	struct node *node;
	struct ivec widim;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *sql;
	long l;

	widim.iv_w = widim.iv_h = widim.iv_d = 0;

	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_state = JST_UNACC;
						node->n_fillp = &jstates[JST_UNACC].js_fill;
						node->n_flags |= NF_HIDE;
					}

	if ((len = asprintf(&sql,
	    " SELECT					"
#define F_nid		0
	    "	processor_id,				"
#define F_n		1
	    "	cpu,					"
#define F_m		2
	    "	slot,					"
#define F_cg		3
	    "	cage,					"
#define F_cb		4
	    "	cab_position,				"
#define F_r		5
	    "	cab_row,				"
#define F_status	6
	    "	processor_status,			"
#define F_x		7
	    "	x_coord,				"
#define F_y		8
	    "	y_coord,				"
#define F_z		9
	    "	z_coord,				"
#define F_type		10
	    "	processor_type				"
	    " FROM					"
	    "	processor				")) == -1)
		err(1, "asprintf");

	if (mysql_real_query(&dbh.dbh_mysql, sql, len))
		err(1, "%s", dbh_error(&dbh));
	free(sql);

	res = mysql_store_result(&dbh.dbh_mysql);
	if (!res)
		err(1, "%s", dbh_error(&dbh));
	while ((row = mysql_fetch_row(res))) {
		if ((l = strtol(row[F_r], NULL, 10)) < 0 || l >= NROWS)
			errx(1, "bad row from database");
		r = (int)l;

		if ((l = strtol(row[F_cb], NULL, 10)) < 0 || l >= NCABS)
			errx(1, "bad cab from database");
		cb = (int)l;

		if ((l = strtol(row[F_cg], NULL, 10)) < 0 || l >= NCAGES)
			errx(1, "bad cage from database");
		cg = (int)l;

		if ((l = strtol(row[F_m], NULL, 10)) < 0 || l >= NMODS)
			errx(1, "bad mod from database");
		m = (int)l;

		if ((l = strtol(row[F_n], NULL, 10)) < 0 || l >= NNODES)
			errx(1, "bad node from database");
		n = (int)l;

		if ((l = strtol(row[F_nid], NULL, 10)) < 0 || l > NID_MAX)
			errx(1, "bad nid from database");
		nid = (int)l;

		if ((l = strtol(row[F_x], NULL, 10)) < 0 || l >= WIDIM_WIDTH)
			errx(1, "bad x from database");
		x = (int)l;

		if ((l = strtol(row[F_y], NULL, 10)) < 0 || l >= WIDIM_HEIGHT)
			errx(1, "bad y from database");
		y = (int)l;

		if ((l = strtol(row[F_z], NULL, 10)) < 0 || l >= WIDIM_DEPTH)
			errx(1, "bad z from database: %s", row[F_z]);
		z = (int)l;

		node = &nodes[r][cb][cg][m][n];
		node->n_nid = nid;
		invmap[nid] = node;
		wimap[x][y][z] = node;
		node->n_wiv.iv_x = x;
		node->n_wiv.iv_y = y;
		node->n_wiv.iv_z = z;

		if (x > widim.iv_w)
			widim.iv_w = x;
		if (y > widim.iv_h)
			widim.iv_h = y;
		if (z > widim.iv_d)
			widim.iv_d = z;

		db_map_set(db_status_map, row[F_status], &node->n_state);
		db_map_set(db_type_map, row[F_type], &node->n_state);

		node->n_fillp = &jstates[node->n_state].js_fill;
		node->n_flags &= ~NF_HIDE;
	}
	mysql_free_result(res);

	if (++widim.iv_w != WIDIM_WIDTH ||
	    ++widim.iv_h != WIDIM_HEIGHT ||
	    ++widim.iv_d != WIDIM_DEPTH)
		errx(1, "wired cluster dimensions have changed");
}

void
db_jobmap(void)
{
}

void
db_tempmap(void)
{
}

void
db_badmap(void)
{
}

void
db_checkmap(void)
{
}

void
db_qstat(void)
{
}

void
db_failmap(void)
{
}
