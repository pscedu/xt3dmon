/* $Id$ */

#include "mon.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cdefs.h"
#include "cam.h"
#include "capture.h"
#include "draw.h"
#include "ds.h"
#include "env.h"
#include "gl.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "queue.h"
#include "selnode.h"
#include "server.h"
#include "state.h"

#define PORT		24242
#define Q		128
#define MSGSIZ		1024

#define SERVP_DONE	0
#define SERVP_CONT	1
#define SERVP_ERR	2

#define NREADTHR	5

int server_mode;
int nsessions, nreqs;
struct client_session *curses;
pthread_t listenthr;

TAILQ_HEAD(, client_session) renderq = TAILQ_HEAD_INITIALIZER(renderq);
TAILQ_HEAD(, client_session) readq = TAILQ_HEAD_INITIALIZER(readq);

pthread_mutex_t renderq_lock	= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
pthread_mutex_t readq_lock	= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

pthread_cond_t renderq_waitq	= PTHREAD_COND_INITIALIZER;
pthread_cond_t readq_waitq	= PTHREAD_COND_INITIALIZER;

int
svc_sw(char *t, int *used, struct client_session *cs)
{
	int new;

	if (sscanf(t, "%d%n", &new, used) != 1)
		return (0);
#define MAXSCRWIDTH 1600
	if (new < 1 || new > MAXSCRWIDTH)
		return (0);
	cs->cs_winv.iv_w = new;
	return (1);
}

int
svc_sh(char *t, int *used, struct client_session *cs)
{
	int new;

	if (sscanf(t, "%d%n", &new, used) != 1)
		return (0);
#define MAXSCRHEIGHT 900
	if (new < 1 || new > MAXSCRHEIGHT)
		return (0);
	cs->cs_winv.iv_h = new;
	return (1);
}

int
svc_x(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%f%n", &cs->cs_v.fv_x, used) != 1)
		return (0);
	return (1);
}

int
svc_y(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%f%n", &cs->cs_v.fv_y, used) != 1)
		return (0);
	return (1);
}

int
svc_z(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%f%n", &cs->cs_v.fv_z, used) != 1)
		return (0);
	return (1);
}

int
svc_lx(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%f%n", &cs->cs_lv.fv_x, used) != 1)
		return (0);
	return (1);
}

int
svc_ly(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%f%n", &cs->cs_lv.fv_y, used) != 1)
		return (0);
	return (1);
}

int
svc_lz(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%f%n", &cs->cs_lv.fv_z, used) != 1)
		return (0);
	return (1);
}

int
svc_job(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%d%n", &cs->cs_jobid, used) != 1) {
		cs->cs_jobid = 0;
		return (0);
	}
	return (1);
}

int
svc_clicku(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%d%n", &cs->cs_mousev.iv_x, used) != 1)
		return (0);
	cs->cs_click++;
	return (1);
}

int
svc_clickv(char *t, int *used, struct client_session *cs)
{
	if (sscanf(t, "%d%n", &cs->cs_mousev.iv_y, used) != 1)
		return (0);
	cs->cs_click++;
	return (1);
}

/*
 * Because the way strncmp() is used, longer
 * keywords with identical prefixes as other keywords
 * must be ordered here first.
 *
 * Tables are NULL-terminated in sve_name.
 */
struct svc_enum {
	const char	*sve_name;
	int		 sve_value;
};

struct svc_enum *
sve_find(const char *s, struct svc_enum *tab, int *used)
{
	struct svc_enum *sve;

	for (sve = tab; sve->sve_name != NULL; sve++)
		if (strncmp(s, sve->sve_name, strlen(sve->sve_name)) == 0) {
			*used = strlen(sve->sve_name);
			return (sve);
		}
	return (NULL);
}

int
svc_hl(char *t, int *used, struct client_session *cs)
{
	static struct svc_enum *sve, tab[] = {
		{ "free",	SC_FREE },
		{ "disabled",	SC_DISABLED },
		{ "down",	SC_DOWN },
		{ "service",	SC_SVC },
		{ NULL,		0 }
	};

	if ((sve = sve_find(t, tab, used)) == NULL)
		return (0);
	cs->cs_opts |= OP_SKEL;
	cs->cs_hlnc = sve->sve_value;
	return (1);
}

int
svc_vmode(char *t, int *used, struct client_session *cs)
{
	static struct svc_enum *sve, tab[] = {
		{ "wiredone",	VM_WIONE },
		{ "wired",	VM_WIRED },
		{ "physical",	VM_PHYS },
		{ NULL,		0 }
	};

	if ((sve = sve_find(t, tab, used)) == NULL)
		return (0);
	cs->cs_vmode = sve->sve_value;
	cs->cs_rf |= RF_VMODE;
	return (1);
}

int
svc_dmode(char *t, int *used, struct client_session *cs)
{
	static struct svc_enum *sve, tab[] = {
		{ "temp",	DM_TEMP },
		{ "jobs",	DM_JOB },
		{ "yods",	DM_YOD },
		{ NULL,		0 }
	};

	if ((sve = sve_find(t, tab, used)) == NULL)
		return (0);
	cs->cs_dmode = sve->sve_value;
	cs->cs_rf |= RF_DMODE;
	return (1);
}

int
sid_valid(const char *sid)
{
	const char *s;

	for (s = sid; *s != '\0'; s++)
		if (!isalnum(*s))
			return (0);
	if (s - sid == SID_LEN)
		return (1);
	return (0);
}

int
svc_sid(char *t, int *used, struct client_session *cs)
{
	char fmtbuf[10];

	snprintf(fmtbuf, sizeof(fmtbuf), "%%%zds%%n",
	    sizeof(cs->cs_sid) - 1);
	if (sscanf(t, fmtbuf, cs->cs_sid, used) != 1)
		return (0);
	if (!sid_valid(cs->cs_sid))
		return (0);
	return (1);
}

struct sv_cmd {
	const char	 *svc_name;
	int		(*svc_act)(char *, int *, struct client_session *);
} sv_cmds[] = {
	{ "sw",		svc_sw },
	{ "sh",		svc_sh },
	{ "x",		svc_x },
	{ "y",		svc_y },
	{ "z",		svc_z },
	{ "lx",		svc_lx },
	{ "ly",		svc_ly },
	{ "lz",		svc_lz },
	{ "job",	svc_job },
	{ "hl",		svc_hl },
	{ "clicku",	svc_clicku },
	{ "clickv",	svc_clickv },
	{ "vmode",	svc_vmode },
	{ "smode",	svc_dmode },
	{ "sid",	svc_sid }
};

int
svc_cmp(const void *a, const void *b)
{
	const struct sv_cmd *ca = a, *cb = b;

	return (strcmp(ca->svc_name, cb->svc_name));
}

int
svc_findcmp(const void *k, const void *elem)
{
	const char *field = ((const struct sv_cmd *)elem)->svc_name;

	return (strcmp(k, field));
}

int
serv_parse(char *s, struct client_session *cs)
{
	struct sv_cmd *svc;
	char *t, *p, *q;
	int len;

	for (t = s; *t != '\0'; t++) {
		while (isspace(*t))
			t++;
		if (*t == '\0')
			break;
//		warnx("cmdbuf: >>>>>%s<<<<<", t);
		if ((p = strchr(t, ':')) == NULL) {
			dbg_warn("No colon in command");
			return (SERVP_ERR);
		}
		for (q = p; q > t && isspace(*--q); )
			;
		*++q = '\0';
		svc = bsearch(t, sv_cmds, sizeof(sv_cmds) / sizeof(sv_cmds[0]),
		   sizeof(sv_cmds[0]), svc_findcmp);
		if (svc == NULL) {
			dbg_warn("Unknown command: %s", t);
			return (SERVP_ERR);
		}
		dbg_warn(" Parsed '%s' command", svc->svc_name);
		t = p;
		while (isspace(*++t))
			;
		if (!svc->svc_act(t, &len, cs)) {
			warnx("Error in value: %s [%s]", t,
			    svc->svc_name);
			return (SERVP_ERR);
		}
		t += len;
		if (*t != '\n') {
			dbg_warn("EOL expected: %s", t);
			return (SERVP_ERR);
		}
	}
	return (SERVP_CONT);
}

void
serv_displayh(void)
{
	static char buf[MSGSIZ];
	struct client_session *cs;
	struct timespec ts;
	struct timeval tv;
	int rf, rv;

	pthread_mutex_lock(&renderq_lock);
	cs = TAILQ_FIRST(&renderq);
	if (cs)
		TAILQ_REMOVE(&renderq, cs, cs_entry);
	pthread_mutex_unlock(&renderq_lock);

	if (cs == NULL) {
		snprintf(buf, sizeof(buf),
		    "xt3dmon server, %d request(s), %d new session(s)",
		    nreqs, nsessions);
		draw_info(buf);
		glutSwapBuffers();

		pthread_mutex_lock(&renderq_lock);
		if (TAILQ_FIRST(&renderq) == NULL) {
			if (gettimeofday(&tv, NULL) == -1)
				err(1, "gettimeofday");
			ts.tv_sec = tv.tv_sec;
			ts.tv_nsec = tv.tv_usec * 1000 + 3e5;
			rv = pthread_cond_timedwait(&renderq_waitq,
			    &renderq_lock, &ts);
			if (rv != 0 && rv != ETIMEDOUT)
				err(1, "pthread_cond_timedwait: %s",
				    strerror(rv));
		}
		pthread_mutex_unlock(&renderq_lock);
		return;
	}

	/* Reset some things for the new session. */
	st.st_opts &= ~(OP_SKEL);
	/* XXX: use opt_disable() */

	sn_clear();
	nc_runall(fill_setopaque);
	caption_set(NULL);

	st.st_rf |= cs->cs_rf;
	st.st_opts |= cs->cs_opts;
	st.st_dmode = cs->cs_dmode;
	st.st_vmode = cs->cs_vmode;
	st.st_v = cs->cs_v;
	st.st_lv = cs->cs_lv;
	mousev = cs->cs_mousev;
	winv = cs->cs_winv;

	if (cs->cs_hlnc != NC_INVAL)
		nc_set(cs->cs_hlnc); /* XXX: ensure job mode */

	dbg_warn("Rendering environment");
	glutReshapeWindow(winv.iv_w, winv.iv_h);
	gl_reshapeh(winv.iv_w, winv.iv_h);

	rf = st.st_rf | RF_CAM | RF_DATASRC | RF_CLUSTER | RF_DMODE;
	if (cs->cs_sid) {
		int dsm;

		if (!dsc_exists(cs->cs_sid)) {
			nsessions++;
			dsc_clone(DS_NODE, cs->cs_sid);
			dsc_clone(DS_JOB, cs->cs_sid);
			dsc_clone(DS_YOD, cs->cs_sid);
		}
		if (!dsc_load(DS_NODE, cs->cs_sid))
			goto drop;
		if (!dsc_load(DS_YOD, cs->cs_sid))
			goto drop;

		dsm = st_dsmode();
		if (dsm == DS_JOB)
			if (!dsc_load(dsm, cs->cs_sid))
				goto drop;
		rf &= ~RF_DATASRC;

		panel_rebuild(PANEL_DATE);
		caption_setdrain(mach_drain);
	}

	/* Have fresh data for (a) jobs and (b) node selection. */
	st.st_rf = 0;
	rebuild(rf_deps(rf));
	if (rf & RF_CAM) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
		glMatrixMode(GL_MODELVIEW);
		cam_look();
		rf &= ~RF_CAM;
	}

	if (cs->cs_jobid) {
		int pos;

		if (job_findbyid(cs->cs_jobid, &pos) != NULL) {
			st.st_opts |= OP_SKEL;
			nc_set(NSC + pos);
			/* Clicking needs clustre rebuild. */
			rebuild(rf_deps(RF_CLUSTER));
		}
	}
	memset(buf, 0, sizeof(buf));
	if (cs->cs_click) {
		gl_select(0);
		panel_hide(PANEL_NINFO);

		if (!SLIST_EMPTY(&selnodes))
			snprintf(buf, sizeof(buf), "nid: %d\n",
			    SLIST_FIRST(&selnodes)->sn_nodep->n_nid);
	}
	/* response terminated by empty line */
	strncat(buf, "\n", sizeof(buf) - 1);
	if (write(cs->cs_fd, buf, sizeof(buf)) != sizeof(buf))
		err(1, "write");

	curses = cs;
	gl_displayh_default();
	curses = NULL;

	capture_snapfd(cs->cs_fd, CM_PNG);
drop:
	dbg_warn("Closing connection\n");
	close(cs->cs_fd);
	free(cs);
}

void *
serv_readthr_main(__unused void *arg)
{
	struct client_session *cs;
	struct timeval tv;
	char buf[MSGSIZ];
	ssize_t len;
	fd_set set;
	int rv;

	for (;;) {
		pthread_mutex_lock(&readq_lock);
		do {
			cs = TAILQ_FIRST(&readq);
			if (cs == NULL)
				pthread_cond_wait(&readq_waitq, &readq_lock);
		} while (cs == NULL);
		TAILQ_REMOVE(&readq, cs, cs_entry);
		pthread_mutex_unlock(&readq_lock);

		rv = SERVP_ERR;
		do {
			FD_ZERO(&set);
			FD_SET(cs->cs_fd, &set);

			tv.tv_sec = 3;
			tv.tv_usec = 0;

			rv = select(cs->cs_fd + 1,
			    &set, NULL, NULL, &tv);
			if (rv == -1)
				err(1, "select");
			if (rv == 0)
				goto drop;
			/*
			 * XXX linux may still block, use
			 * O_NONBLOCK and drop on EWOULDBLOCK.
			 */
			if ((len = read(cs->cs_fd, buf, sizeof(buf) - 1)) == -1)
				err(1, "read");
			if (len == 0)
				break;
			buf[len] = '\0';

			rv = serv_parse(buf, cs);
		} while (rv == SERVP_CONT);
		if (rv == SERVP_ERR)
			goto drop;

		pthread_mutex_lock(&renderq_lock);
		TAILQ_INSERT_TAIL(&renderq, cs, cs_entry);
		pthread_cond_signal(&renderq_waitq);
		pthread_mutex_unlock(&renderq_lock);
		sched_yield();
		continue;
 drop:
		close(cs->cs_fd);
		free(cs);
		sched_yield();
	}
}

void *
serv_listenthr_main(__unused void *arg)
{
	struct client_session *cs;
	struct sockaddr_in sa_in;
	int fd, optval, s;
	socklen_t sz;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket");
	optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval,
	    sizeof(optval)) == -1)
		err(1, "setsockopt");

	memset(&sa_in, 0, sizeof(sa_in));
#ifdef HAVE_SA_LEN
	sa_in.sin_len = sizeof(sa_in);
#endif
	sa_in.sin_family = AF_INET;
	sa_in.sin_port = htons(PORT);
	sa_in.sin_addr.s_addr = htonl(INADDR_ANY);
	sz = sizeof(sa_in);
	if (bind(s, (struct sockaddr *)&sa_in, sz) == -1)
		err(1, "bind");
	if (listen(s, Q) == -1)
		err(1, "listen");

	for (;;) {
		sz = 0;
		if ((fd = accept(s, (struct sockaddr *)&sa_in, &sz)) == -1)
			err(1, "accept");
		dbg_warn("Servicing new connection");
		nreqs++;

		cs = malloc(sizeof(*cs));
		if (cs == NULL)
			err(1, "malloc");
		memset(cs, 0, sizeof(*cs));
		cs->cs_fd = fd;
		cs->cs_hlnc = NC_INVAL;

		pthread_mutex_lock(&readq_lock);
		TAILQ_INSERT_TAIL(&readq, cs, cs_entry);
		pthread_cond_signal(&readq_waitq);
		pthread_mutex_unlock(&readq_lock);
		sched_yield();
	}
}

void
serv_init(void)
{
	struct panel *p;
	pthread_t *thr;
	int rv, i;

	st.st_opts &= ~(OP_TWEEN | OP_NODEANIM);
	st.st_opts |= OP_NLABELS;

	panel_toggle(PANEL_DATE);
	if ((p = panel_for_id(PANEL_DATE)) != NULL) {
		p->p_u = 0;
		p->p_v = 40; /* XXX: hack to font size */
		p->p_w = 110;
		p->p_h = 40;
	}

	fill_bg.f_r = 0.1f;
	fill_bg.f_g = 0.2f;
	fill_bg.f_b = 0.3f;

	qsort(sv_cmds, sizeof(sv_cmds) / sizeof(sv_cmds[0]),
	    sizeof(sv_cmds[0]), svc_cmp);
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		err(1, "signal");

	if ((rv = pthread_create(&listenthr, NULL,
	    serv_listenthr_main, NULL)) != 0)
		errx(1, "pthread_create: %s", strerror(rv));
	for (i = 0; i < NREADTHR; i++) {
		thr = malloc(sizeof(*thr));
		if (thr == NULL)
			err(1, "malloc");
		if ((rv = pthread_create(thr, NULL,
		    serv_readthr_main, NULL)) != 0)
			errx(1, "pthread_create: %s", strerror(rv));
	}
}
