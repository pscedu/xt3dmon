/* $Id$ */

#include "mon.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "cdefs.h"
#include "cam.h"
#include "capture.h"
#include "ds.h"
#include "env.h"
#include "gl.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "selnode.h"
#include "server.h"
#include "state.h"

#define PORT	24242
#define BACKLOG	128
#define USLEEP	100
#define MSGSIZ	1024

void serv_displayh(void);
int serv_parse(char *, struct session *);

int svc_sw(char *, int *, struct session *);
int svc_sh(char *, int *, struct session *);
int svc_x(char *, int *, struct session *);
int svc_y(char *, int *, struct session *);
int svc_z(char *, int *, struct session *);
int svc_lx(char *, int *, struct session *);
int svc_ly(char *, int *, struct session *);
int svc_lz(char *, int *, struct session *);
int svc_job(char *, int *, struct session *);
int svc_hl(char *, int *, struct session *);
int svc_clicku(char *, int *, struct session *);
int svc_clickv(char *, int *, struct session *);
int svc_vmode(char *, int *, struct session *);
int svc_dmode(char *, int *, struct session *);
int svc_sid(char *, int *, struct session *);

struct sv_cmd {
	const char	 *svc_name;
	int		(*svc_act)(char *, int *, struct session *);
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

int sock;
int nsessions, nreqs;
struct session *ssp;

int
svc_cmp(const void *a, const void *b)
{
	return (strcmp(((const struct sv_cmd *)a)->svc_name,
	    ((const struct sv_cmd *)b)->svc_name));
}

int
svc_findcmp(const void *k, const void *elem)
{
	const char *field = ((const struct sv_cmd *)elem)->svc_name;

	return (strcmp((const char *)k, field));
}

void
serv_init(void)
{
	struct sockaddr_in sin;
	struct panel *p;
	socklen_t sz;
	int optval;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket");
	optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval,
	    sizeof(optval)) == -1)
		err(1, "setsockopt");

/*
	int fflags;
	if (fcntl(sock, F_GETFL, &fflags) == -1)
		err(1, "fcntl F_GETFL");
	fflags |= O_NONBLOCK;
*/
	if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
		err(1, "fcntl F_SETFL");

	memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SA_LEN
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sz = sizeof(sin);
	if (bind(sock, (struct sockaddr *)&sin, sz) == -1)
		err(1, "bind");
	if (listen(sock, BACKLOG) == -1)
		err(1, "listen");

	st.st_opts &= ~(OP_TWEEN | OP_NODEANIM);
	st.st_opts |= OP_NLABELS;

	panel_toggle(PANEL_DATE);
	if ((p = panel_for_id(PANEL_DATE)) != NULL) {
		p->p_stick = PSTICK_BL;
		p->p_u = 0;
		p->p_v = 40; /* XXX: hack to font size */
		p->p_w = 80;
		p->p_h = 40;
	}

	gl_displayhp = serv_displayh;
//	rebuild(RF_DATASRC | RF_CLUSTER);
//	st.st_rf &= ~(RF_DATASRC | RF_CLUSTER);
	fill_bg.f_r = 0.0;
	fill_bg.f_g = 0.0;
	fill_bg.f_b = 0.0;

	qsort(sv_cmds, sizeof(sv_cmds) / sizeof(sv_cmds[0]),
	    sizeof(sv_cmds[0]), svc_cmp);
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		err(1, "signal");
}

#define SERVP_DONE	0
#define SERVP_CONT	1
#define SERVP_ERR	2

#define MAXTRIES 10
#define TRYWAIT 10	/* microseconds */

void
serv_drawinfo(void)
{
	char *p, buf[BUFSIZ];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, winv.iv_w, 0.0, winv.iv_h);

	glColor3f(1.0f, 1.0f, 1.0f);

	snprintf(buf, sizeof(buf),
	    "xt3dmon server, %d request(s), %d new session(s)",
	    nreqs, nsessions);

	glRasterPos2d(3, 12);
	for (p = buf; *p != '\0'; p++)
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	glutSwapBuffers();
}

void
serv_displayh(void)
{
	struct sockaddr_in sin;
	struct session ss;
	char buf[MSGSIZ];
	int i, clifd, rf;
	socklen_t sz;
	ssize_t len;

	serv_drawinfo();

	/* Reset some things for the new session. */
	st.st_opts &= ~(OP_SKEL);

	sz = 0;
	memset(&ss, 0, sizeof(ss));
	if ((clifd = accept(sock, (struct sockaddr *)&sin, &sz)) == -1) {
		if (errno != EWOULDBLOCK &&
		    errno != EAGAIN &&
		    errno != EINTR)
			err(1, "accept");
		usleep(USLEEP);
		return;
	}
	dbg_warn("Servicing new connection");
	nreqs++;
	sn_clear();
	hl_restoreall();
	for (i = 0; i < MAXTRIES; i++) {
		usleep(TRYWAIT);
		if ((len = read(clifd, buf, sizeof(buf) - 1)) == -1) {
			if (errno != EWOULDBLOCK &&
			    errno != EAGAIN &&
			    errno != EINTR)
				err(1, "read");
			continue;
		}
		i = 0;
		if (len == 0)
			break;
		buf[len] = '\0';
		dbg_warn("Parsing input");
		switch (serv_parse(buf, &ss)) {
		case SERVP_ERR:
//			dbg_warn("Error encountered", buf);
			goto drop;
		case SERVP_DONE:
			goto snap;
		}
	}
	if (i == MAXTRIES)
		goto drop;
snap:
	dbg_warn("Writing snapshot");
//	winv.iv_h += 20;		/* XXX: X title bar slack */
//	glutReshapeFunc(NULL);
	glutReshapeWindow(winv.iv_w, winv.iv_h);
//	glutReshapeFunc(gl_reshapeh);
	gl_reshapeh(winv.iv_w, winv.iv_h);

	rf = st.st_rf | RF_CAM | RF_DATASRC | RF_CLUSTER | RF_DMODE;
	if (ss.ss_sid) {
		struct panel *p;
		int dsm;

		if (!dsc_exists(ss.ss_sid)) {
			nsessions++;
			dsc_clone(DS_NODE, ss.ss_sid);
			dsc_clone(DS_JOB, ss.ss_sid);
			dsc_clone(DS_YOD, ss.ss_sid);
		}
		dsc_load(DS_NODE, ss.ss_sid);

		dsm = st_dsmode();
		if (dsm != DS_INV)
			dsc_load(dsm, ss.ss_sid);
		rf &= ~RF_DATASRC;

		if ((p = panel_for_id(PANEL_DATE)) != NULL)
			p->p_opts |= POPT_USRDIRTY;
	}

	/* Have fresh data for (a) jobs and (b) node selection. */
	st.st_rf = 0;
	rebuild(rf);
	if (rf & RF_CAM) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
		glMatrixMode(GL_MODELVIEW);
		cam_look();
		rf &= ~RF_CAM;
	}

	if (ss.ss_jobid) {
		struct job *j;

		if ((j = job_findbyid(ss.ss_jobid)) != NULL) {
			st.st_opts |= OP_SKEL;
			st.st_rf |= RF_CLUSTER;

			hl_clearall();
			job_hl(j);
		}
	}
	memset(buf, 0, sizeof(buf));
	if (ss.ss_click) {
		gl_displayhp_old = serv_displayh;
		gl_displayh_select();
		panel_hide(PANEL_NINFO);

		if (!SLIST_EMPTY(&selnodes))
			snprintf(buf, sizeof(buf), "nid: %d\n",
			    SLIST_FIRST(&selnodes)->sn_nodep->n_nid);
	}
	/* response terminated by empty line */
	strncat(buf, "\n", sizeof(buf) - 1);
	if (write(clifd, buf, sizeof(buf)) != sizeof(buf))
		err(1, "write");

	gl_displayhp_old = serv_displayh;
	ssp = &ss;
	gl_displayh_default();
	ssp = NULL;

	capture_snapfd(clifd, CM_PNG);
drop:
	dbg_warn("Closing connection\n");
	close(clifd);
}

int
serv_parse(char *s, struct session *ss)
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
		if (!svc->svc_act(t, &len, ss)) {
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

#define MAXWIDTH 1600
#define MAXHEIGHT 900

int
svc_sw(char *t, int *used, __unused struct session *ss)
{
	int new;

	if (sscanf(t, "%d%n", &new, used) != 1)
		return (0);
	if (new < 1 || new > MAXWIDTH)
		return (0);
	winv.iv_w = new;
	return (1);
}

int
svc_sh(char *t, int *used, __unused struct session *ss)
{
	int new;

	if (sscanf(t, "%d%n", &new, used) != 1)
		return (0);
	if (new < 1 || new > MAXHEIGHT)
		return (0);
	winv.iv_h = new;
	return (1);
}

int
svc_x(char *t, int *used, __unused struct session *ss)
{
	if (sscanf(t, "%f%n", &st.st_v.fv_x, used) != 1)
		return (0);
	return (1);
}

int
svc_y(char *t, int *used, __unused struct session *ss)
{
	if (sscanf(t, "%f%n", &st.st_v.fv_y, used) != 1)
		return (0);
	return (1);
}

int
svc_z(char *t, int *used, __unused struct session *ss)
{
	if (sscanf(t, "%f%n", &st.st_v.fv_z, used) != 1)
		return (0);
	return (1);
}

int
svc_lx(char *t, int *used, __unused struct session *ss)
{
	if (sscanf(t, "%f%n", &st.st_lv.fv_x, used) != 1)
		return (0);
	return (1);
}

int
svc_ly(char *t, int *used, __unused struct session *ss)
{
	if (sscanf(t, "%f%n", &st.st_lv.fv_y, used) != 1)
		return (0);
	return (1);
}

int
svc_lz(char *t, int *used, __unused struct session *ss)
{
	if (sscanf(t, "%f%n", &st.st_lv.fv_z, used) != 1)
		return (0);
	return (1);
}

int
svc_job(char *t, int *used, struct session *ss)
{
	if (sscanf(t, "%d%n", &ss->ss_jobid, used) != 1) {
		ss->ss_jobid = 0;
		return (0);
	}
	return (1);
}

int
svc_clicku(char *t, int *used, struct session *ss)
{
	if (sscanf(t, "%d%n", &mousev.iv_x, used) != 1)
		return (0);
	ss->ss_click++;
	return (1);
}

int
svc_clickv(char *t, int *used, struct session *ss)
{
	if (sscanf(t, "%d%n", &mousev.iv_y, used) != 1)
		return (0);
	ss->ss_click++;
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
svc_hl(char *t, int *used, __unused struct session *ss)
{
	struct svc_enum *sve, tab[] = {
		{ "free",	SC_FREE },
		{ "disabled",	SC_DISABLED },
		{ "down",	SC_DOWN },
		{ "service",	SC_SVC },
		{ NULL,		0 }
	};

	if ((sve = sve_find(t, tab, used)) == NULL)
		return (0);
	hl_state(sve->sve_value);
	st.st_opts |= OP_SKEL;
	st.st_rf |= RF_CLUSTER;
	return (1);
}

int
svc_vmode(char *t, int *used, __unused struct session *ss)
{
	struct svc_enum *sve, tab[] = {
		{ "wiredone",	VM_WIREDONE },
		{ "wired",	VM_WIRED },
		{ "physical",	VM_PHYSICAL },
		{ NULL,		0 }
	};

	if ((sve = sve_find(t, tab, used)) == NULL)
		return (0);
	st.st_vmode = sve->sve_value;
	st.st_rf |= RF_CLUSTER | RF_CAM | RF_GROUND | RF_SELNODE;
	return (1);
}

int
svc_dmode(char *t, int *used, __unused struct session *ss)
{
	struct svc_enum *sve, tab[] = {
		{ "temp",	DM_TEMP },
		{ "jobs",	DM_JOB },
		{ "yods",	DM_YOD },
		{ "fail",	DM_FAIL },
		{ NULL,		0 }
	};

	if ((sve = sve_find(t, tab, used)) == NULL)
		return (0);
	st.st_dmode = sve->sve_value;
	st.st_rf |= RF_DMODE;
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
svc_sid(char *t, int *used, __unused struct session *ss)
{
	char fmtbuf[10];

	snprintf(fmtbuf, sizeof(fmtbuf), "%%%ds%%n",
	    sizeof(ss->ss_sid) - 1);
	if (sscanf(t, fmtbuf, ss->ss_sid, used) != 1)
		return (0);
	if (!sid_valid(ss->ss_sid))
		return (0);
	return (1);
}
