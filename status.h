/* $Id$ */

#define SLP_URGENT	0
#define SLP_NOTICE	1

struct status_log {
	int			 sl_prio;
	char			*sl_str;
	TAILQ_ENTRY(status_log)	 sl_link;
};

TAILQ_HEAD(status_hd, status_log);

void status_add(int, const char *, ...);

extern struct status_hd status_hd;
