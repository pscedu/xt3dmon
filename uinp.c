/* $Id$ */

#include "mon.h"

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "capture.h"
#include "flyby.h"
#include "gl.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "pathnames.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "uinp.h"
#include "util.h"

char authbuf[BUFSIZ];

void
uinpcb_cmd(void)
{
	/* Send command to service node. */
}

/* Screenshot user input callback. */
void
uinpcb_ss(void)
{
	const char *s, *ext;
	int cm;

	s = buf_get(&uinp.uinp_buf);

	if (s[0] == '\0')
		return;

	if ((ext = strrchr(s, '.')) == NULL) {
		buf_chop(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '.');
		buf_append(&uinp.uinp_buf, 'p');
		buf_append(&uinp.uinp_buf, 'n');
		buf_append(&uinp.uinp_buf, 'g');
		buf_append(&uinp.uinp_buf, '\0');
		cm = CM_PNG;
	} else {
		if (strcmp(ext, ".png") == 0)
			cm = CM_PNG;
		else
			/* XXX: status_add("unsupported"); */
			cm = CM_PPM;
	}
	capture_snap(s, cm);
}

void
uinpcb_gotonode(void)
{
	struct node *n;
	char *s;
	int nid;
	long l;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 10);
	if (l < 0 || l > NID_MAX || !isdigit(*s))
		return;
	nid = (int)l;

	if ((n = node_for_nid(nid)) == NULL)
		return;
	sn_add(n, &fv_zero);
	node_goto(n);
	panel_show(PANEL_NINFO);
}

void
uinpcb_gotojob(void)
{
	int jobid, pos;
	char *s;
	long l;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 10);
	if (l <= 0 || !isdigit(*s))
		return;
	jobid = (int)l;

	if (job_findbyid(jobid, &pos) == NULL)
		return;
	nc_set(NSC + pos);

	if (st.st_dmode != DM_JOB) {
		st.st_rf |= RF_DMODE;
		st.st_dmode = DM_JOB;
	}
}

void
uinpcb_eggs(void)
{
	char *cmd;

	cmd = buf_get(&uinp.uinp_buf);

	/* Parse Easter egg keywords =) */
	if (strcmp(cmd, "borg") == 0)
		egg_toggle(EGG_BORG);
	else if (strcmp(cmd, "matrix") == 0)
		egg_toggle(EGG_MATRIX);
}

void
uinpcb_login(void)
{
	struct pinfo *pi;
	struct panel *p;
	size_t siz;
	char *s;

	s = buf_get(&uinp.uinp_buf);

	if ((p = panel_for_id(PANEL_LOGIN)) != NULL) {
		siz = sizeof(authbuf) - 1;
		if (p->p_opts & POPT_LOGIN_ATPASS) {
			strncat(authbuf, ":", siz - strlen(authbuf));
			authbuf[siz] = '\0';

			strncat(authbuf, s, siz - strlen(authbuf));
			authbuf[siz] = '\0';

			panel_tremove(p);
			st.st_rf |= RF_DATASRC;

			if (strlen(authbuf) < 4 * sizeof(login_auth) / 3)
{
 printf("base64: have %d chars\n", sizeof(login_auth));
				base64_encode(authbuf, login_auth,
				    strlen(authbuf));
}
			memset(authbuf, 0, sizeof(authbuf));
		} else {
			memset(login_auth, 0, sizeof(login_auth));

			/* For resetting login_auth. */
			if (strcmp(s, "") == 0) {
				panel_tremove(p);
				st.st_rf |= RF_DATASRC;
				return;
			}

			strncpy(authbuf, s, siz);
			authbuf[siz] = '\0';

			pi = &pinfo[baseconv(p->p_id) - 1];
			p->p_opts |= POPT_LOGIN_ATPASS | POPT_REFRESH;
			glutKeyboardFunc(gl_keyh_uinput);
			uinp.uinp_panel = p;
			uinp.uinp_opts = pi->pi_uinpopts;
			uinp.uinp_callback = pi->pi_uinpcb;
		}
	}
}

void
uinpcb_fbnew(void)
{
	char path[PATH_MAX], *buf, *s;
	struct stat stb;
	FILE *fp;

	buf = buf_get(&uinp.uinp_buf);

	for (s = buf; *s != '\0'; s++)
		if (!isalnum(*s) && *s != ' ' && *s != '.') {
			*s = '\0';
			break;
		}

	/* Create a new flyby data file, and make it selected */
	if (buf[0] != '\0' && buf[0] != '.') {
		snprintf(flyby_name, sizeof(flyby_name), "%s", buf);
		snprintf(path, sizeof(path), "%s/%s",
		    _PATH_FLYBYDIR, flyby_name);

		if (stat(_PATH_FLYBYDIR, &stb) == -1) {
			if (errno == ENOENT) {
				if (mkdir(_PATH_FLYBYDIR, 0755) == -1)
					err(1, "%s", _PATH_FLYBYDIR);
			} else
				err(1, "%s", _PATH_FLYBYDIR);
		}
		if ((fp = fopen(path, "a+")) == NULL)
			err(1, "%s", path);
		fclose(fp);
		errno = 0;
	}
}
