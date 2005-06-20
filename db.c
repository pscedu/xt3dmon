/* $Id$ */

#include <err.h>
#include <stdio.h>

#include <mysql.h>

#define DBH_HOST	"localhost"
#define DBH_USER	"basic"
#define DBH_PASS	"basic"
#define DBH_PORT	3306
#define DBH_SOCK	NULL
#define DBH_DB		"XTAdmin"

struct dbh {
	union {
		MYSQL dbhu_mysql;
	} dbh_u;
#define dbh_mysql dbh_u.dbhu_mysql
};

struct dbh dbh;

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

const char *
dbh_error(struct dbh *dbh)
{
	return (mysql_error(&dbh->dbh_mysql));
}

struct db_map_ent {
	const char	*dme_name;
	int		 dme_code;
	int		 dme_doset;
} db_status_map[] = {
	{ "up",		JST_FREE,	0 }, /* XXX */
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
		if (strcmp(dme->dme_name, dbval) == 0 &&
		    dme->dme_doset) {
			*field = dme->dme_code;
			return;
		}
	warnx("%s: database value not found in map", dbval);
}

void
refresh_physmap(struct dbh *dbh)
{
	int len, r, cb, cg, m, n;
	struct node *node;
	MYSQL_RES res;
	MYSQL_ROW row;

	for (r = 0; r < NROWS; r++)
		for (cb = 0; c < NCABS; cb++)
			for (cg = 0; c < NCAGES; cg++)
				for (m = 0; r < NMODS; m++)
					for (n = 0; r < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_state = JST_UNACC;
						node->n_fillp = &jstates[JST_UNACC].js_fill;
					}

	if ((len = asprintf(&sql,
	    "SELECT					"
#define F_nid 0
	    "	processor_id,				"
#define F_n
	    "	cpu,					"
#define F_m
	    "	slot,					"
#define F_cg
	    "	cage,					"
#define F_cb
	    "	cab_position,				"
#define F_r
	    "	cab_row,				"
#define F_status
	    "	processor_status,			"
#define F_x
	    "	x_coord,				"
#define F_y
	    "	y_coord,				"
#define F_z
	    "	z_coord,				"
#define F_type
	    "	processor_type				")) == -1)
		err(1, "asprintf");

	if (mysql_real_query(&dbh->dbh_mysql, sql, len))
		err(1, "%s", dbh_error(dbh));
	free(sql);

	res = mysql_store_result(&dbh->dbh_mysql);
	if (!res)
		err(1, "%s", dbh_error(dbh));
	while (row = mysql_fetch_row(res)) {
		node = &nodes[row[F_r]][row[F_cb]][row[F_cg]][row[F_m]][row[F_n]];
		node->n_nid = row[F_nid];
		invmap[row[F_nid]] = node;
		node->n_logv.v_x = row[F_x];
		node->n_logv.v_y = row[F_y];
		node->n_logv.v_z = row[F_z];

		if (row[F_x] > logical_width)
			logical_width = row[F_x];
		if (row[F_y] > logical_height)
			logical_height = row[F_y];
		if (row[F_z] > logical_depth)
			logical_depth = row[F_z];

		db_map_set(db_status_map, row[F_status], &node->n_state);
		db_map_set(db_type_map, row[F_type], &node->n_state);

		node->n_fillp = &jstates[node->n_state].js_fill;
	}
	mysql_free_result(res);
}

void
refresh_jobmap(void)
{

}
