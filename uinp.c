/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#include "mon.h"

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "capture.h"
#include "ds.h"
#include "flyby.h"
#include "gl.h"
#include "job.h"
#include "mach.h"
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
	const char *s, *ext, *home;
	struct buf buf;
	int cm;

	s = buf_get(&uinp.uinp_buf);
	if (s[0] == '\0')
		return;

	if (s[0] == '~' && s[1] == '/' &&
	    (home = getenv("HOME")) != NULL) {
		buf_init(&buf);
		buf_appendv(&buf, home);
		buf_appendv(&buf, &s[1]);
		buf_nul(&buf);

		buf_free(&uinp.uinp_buf);
		uinp.uinp_buf = buf;

		s = buf_get(&uinp.uinp_buf);
	}

	if ((ext = strrchr(s, '.')) == NULL) {
		buf_chop(&uinp.uinp_buf);
		buf_appendv(&uinp.uinp_buf, ".png");
		buf_append(&uinp.uinp_buf, '\0');
		cm = CM_PNG;
	} else {
		if (strcmp(ext, ".png") == 0)
			cm = CM_PNG;
		else
			/* XXX: status_add("unsupported"); */
			cm = CM_PPM;
	}
	if (capture_usevirtual)
		capture_virtual(s, cm);
	else
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
	if (l < 0 || l > machine.m_nidmax || !isdigit(*s))
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
	else if (strcmp(cmd, "hackers") == 0)
		egg_toggle(EGG_HACKERS);
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

			if (strlen(authbuf) < 4 * sizeof(login_auth) / 3)
				base64_encode(authbuf, login_auth,
				    strlen(authbuf));
			memset(authbuf, 0, sizeof(authbuf));

			dsfopts |= DSFO_SSL;
			ds_setlive();
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
	char *buf, *s;
	FILE *fp;

	buf = buf_get(&uinp.uinp_buf);

	for (s = buf; *s != '\0'; s++)
		if (!isalnum(*s) && *s != ' ' && *s != '.') {
			*s = '\0';
			break;
		}

	/* Create a new flyby data file, and make it selected */
	if (buf[0] != '\0' && buf[0] != '.') {
		flyby_set(buf, 0);

#if 0
		if (stat(dir, &stb) == -1) {
			if (errno == ENOENT) {
				if (mkdir(_PATH_FLYBYDIR, 0755) == -1)
					err(1, "%s", _PATH_FLYBYDIR);
			} else
				err(1, "%s", _PATH_FLYBYDIR);
		}
#endif

		/* Touch the new flyby file. */
		if ((fp = fopen(flyby_fn, "a+")) == NULL)
			err(1, "%s", flyby_fn);
		fclose(fp);
		errno = 0;
	}
}
