/* $Id$ */

#ifdef __GNUC__
#define _GNU_SOURCE /* asprintf, disgusting */
#endif

#include <sys/types.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "cdefs.h"
#include "mon.h"
#include "queue.h"

#define LETTER_HEIGHT	13
#define LETTER_WIDTH	8
#define PANEL_PADDING	3
#define PANEL_BWIDTH	(2.0f)
#define PWIDGET_LENGTH	15
#define PWIDGET_HEIGHT	LETTER_HEIGHT
#define PWLABEL_LENGTH	(win_width / 4 / 2 - PWIDGET_PADDING)
#define PWIDGET_PADDING	3

void panel_refresh_fps(struct panel *);
void panel_refresh_ninfo(struct panel *);
void panel_refresh_cmd(struct panel *);
void panel_refresh_legend(struct panel *);
void panel_refresh_flyby(struct panel *);
void panel_refresh_goto(struct panel *);

void (*refreshf[])(struct panel *) = {
	panel_refresh_fps,
	panel_refresh_ninfo,
	panel_refresh_cmd,
	panel_refresh_legend,
	panel_refresh_flyby,
	panel_refresh_goto
};

struct panels	 panels;
int		 panel_offset;
int		 goto_logid;

int
baseconv(int n)
{
	unsigned int i;

	for (i = 0; i < sizeof(n) * 8; i++)
		if (n & (1 << i))
			return (i + 1);
	return (0);
}

void
panel_free(struct panel *p)
{
	struct pwidget *pw, *nextp;

	TAILQ_REMOVE(&panels, p, p_link);
	for (pw = SLIST_FIRST(&p->p_widgets);
	    pw != SLIST_END(&p->p_widgets);
	    pw = nextp) {
		nextp = SLIST_NEXT(pw, pw_next);
		free(pw);
	}
	free(p->p_str);
	free(p);
}

void
draw_panel(struct panel *p)
{
	int lineno, curlen, toff, uoff, voff, npw;
	struct pwidget *pw;
	int u, v, w, h;
	char *s;

	toff = PANEL_PADDING + PANEL_BWIDTH;

	if (p->p_opts & POPT_REMOVE) {
		w = 0;
		h = 0;
		u = win_width;
		v = win_height;
	} else {
		lineno = 1;
		curlen = w = 0;
		for (s = p->p_str; *s != '\0'; s++, curlen++) {
			if (*s == '\n') {
				if (curlen > w)
					w = curlen;
				curlen = -1;
				lineno++;
			}
		}
		if (curlen > w)
			w = curlen;
		w = w * LETTER_WIDTH + 2 * toff;
		h = lineno * LETTER_HEIGHT + 2 * toff;
		if (p->p_nwidgets) {
			w = MAX(w, 2 * (LETTER_WIDTH * (int)p->p_maxwlen +
			    PWIDGET_LENGTH + 2 * PWIDGET_PADDING) + PWIDGET_PADDING);
			w = MIN(w, 2 * PWLABEL_LENGTH);
			/* p_nwidgets + 1 for truncation */
			h += ((p->p_nwidgets + 1) / 2) * PWIDGET_HEIGHT +
			    ((p->p_nwidgets + 1) / 2 - 1) * PWIDGET_PADDING;
		}
		u = win_width - w;
		v = win_height - panel_offset;
	}

	if (p->p_u != u)
		p->p_u += p->p_u - u < 0 ? 1 : -1;
	if (p->p_v != v)
		p->p_v += p->p_v - v < 0 ? 1 : -1;
	if (p->p_w != w)
		p->p_w += p->p_w - w < 0 ? 1 : -1;
	if (p->p_h != h)
		p->p_h += p->p_h - h < 0 ? 1 : -1;

	if ((p->p_opts & POPT_REMOVE) && p->p_u >= win_width) {
		panel_free(p);
		return;
	}

	/* Save state and set things up for 2D. */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, win_width, 0.0, win_height);

	glColor4f(p->p_fill.f_r, p->p_fill.f_g, p->p_fill.f_b, p->p_fill.f_a);

	/* Panel content. */
	voff = p->p_v - toff;
	uoff = p->p_u + toff;
	for (s = p->p_str; *s != '\0'; s++) {
		if (*s == '\n' || s == p->p_str) {
			voff -= LETTER_HEIGHT;
			if (voff < p->p_v - p->p_h)
				break;
			glRasterPos2d(uoff, voff);
			if (*s == '\n')
				continue;
		}
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);
	}

	npw = 0;
	uoff += p->p_w / 2;
	voff += PWIDGET_HEIGHT; /* first loop cuts into this */
	SLIST_FOREACH(pw, &p->p_widgets, pw_next) {
		struct fill *fp = pw->pw_fillp;

fp->f_a = 1.00f;

		uoff += p->p_w / 2 * (npw % 2 ? 1 : -1);
		if (npw % 2 == 0)
			voff -= PWIDGET_HEIGHT + PWIDGET_PADDING;
		if (voff - PWIDGET_HEIGHT < p->p_v - p->p_h)
			break;

		/* Draw widget background. */
		glBegin(GL_POLYGON);
		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		glVertex2d(uoff,			voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff - PWIDGET_HEIGHT);
		glVertex2d(uoff,			voff - PWIDGET_HEIGHT);
		glEnd();

		/* Draw widget border. */
		glLineWidth(1.0f);
		glBegin(GL_LINE_LOOP);
		glColor4f(0.00f, 0.00f, 0.00f, 1.00f);
		glVertex2d(uoff,			voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff - PWIDGET_HEIGHT);
		glVertex2d(uoff,			voff - PWIDGET_HEIGHT);
		glEnd();

		glColor4f(p->p_fill.f_r, p->p_fill.f_g, p->p_fill.f_b, p->p_fill.f_a);
		glRasterPos2d(uoff + PWIDGET_LENGTH + PWIDGET_PADDING,
		    voff - PWIDGET_HEIGHT + 3);
		for (s = pw->pw_str; *s != '\0' &&
		    (s - pw->pw_str) * LETTER_WIDTH + PWIDGET_LENGTH +
		    PWIDGET_PADDING * 4 < p->p_w / 2; s++)
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);

		/*
		 * We may have to break early because of the way
		 * persistent memory allocations are performed.
		 */
		if (++npw >= p->p_nwidgets)
			break;
	}

	/* Draw background. */
	glBegin(GL_POLYGON);
	glColor4f(0.20, 0.40, 0.5, 1.0);
	glVertex2d(p->p_u,		p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
	glVertex2d(p->p_u,		p->p_v - p->p_h);
	glEnd();

	/* Draw border. */
	glLineWidth(PANEL_BWIDTH);
	glBegin(GL_LINE_LOOP);
	glColor4f(0.40, 0.80, 1.0, 1.0);
	glVertex2d(p->p_u,		p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
	glVertex2d(p->p_u,		p->p_v - p->p_h);
	glEnd();

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	panel_offset += p->p_h + 3; /* spacing */
}

void
panel_set_content(struct panel *p, char *fmt, ...)
{
	va_list ap;
	size_t len;

	for (;;) {
		va_start(ap, fmt);
		len = vsnprintf(p->p_str, p->p_strlen,
		    fmt, ap);
		va_end(ap);
		if (len >= p->p_strlen) {
			p->p_strlen = len + 1;
			if ((p->p_str = realloc(p->p_str,
			    p->p_strlen)) == NULL)
				err(1, "realloc");
		} else
			break;
	}
}

void
panel_refresh_fps(struct panel *p)
{
	panel_set_content(p, "FPS: %d", fps);
}

void
panel_refresh_cmd(struct panel *p)
{
	if (selnode == NULL)
		panel_set_content(p, "Please select a node\nto send a command to.");
	else
		panel_set_content(p, "Sending command to host\n\n> %s",
		    buf_get(&uinp.uinp_buf));
}

struct pwidget *
panel_get_pwidget(struct panel *p, struct pwidget *pw, struct pwidget **nextp)
{
	if (pw == NULL) {
		if ((pw = malloc(sizeof(*pw))) == NULL)
			err(1, "malloc");
		SLIST_INSERT_HEAD(&p->p_widgets, pw, pw_next);
//		memset(pw, 0, sizeof(pw));
		*nextp = NULL;
	} else
		*nextp = SLIST_NEXT(pw, pw_next);
	p->p_nwidgets++;
	return (pw);
}

void
panel_refresh_legend(struct panel *p)
{
	struct pwidget *pw, *nextp;
	size_t j;

	p->p_nwidgets = 0;
	p->p_maxwlen = 0;
	pw = SLIST_FIRST(&p->p_widgets);
	switch (st.st_mode) {
	case SM_JOBS:
		panel_set_content(p, "Jobs: %d", njobs);
		for (j = 0; j < NJST; j++, pw = nextp) {
			if (j == JST_USED)
				continue;
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &jstates[j].js_fill;
			pw->pw_str = jstates[j].js_name;
			p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		}
		for (j = 0; j < njobs; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &jobs[j]->j_fill;
			pw->pw_str = jobs[j]->j_name;
			p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		}
		break;
	case SM_FAIL:
		panel_set_content(p, "Failures: %d", total_failures);
		pw = panel_get_pwidget(p, pw, &nextp);
		pw->pw_fillp = &fail_notfound.f_fill;
		pw->pw_str = fail_notfound.f_name;
		p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		pw = nextp;
		for (j = 0; j < nfails; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &fails[j]->f_fill;
			pw->pw_str = fails[j]->f_name;
			p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		}
		break;
	case SM_TEMP:
		panel_set_content(p, "Temperature");
		pw = panel_get_pwidget(p, pw, &nextp);
		pw->pw_fillp = &temp_notfound.t_fill;
		pw->pw_str = temp_notfound.t_name;
		p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		pw = nextp;
		for (j = 0; j < ntemps; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &temps[j]->t_fill;
			pw->pw_str = temps[j]->t_name;
			p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		}
		break;
	}
}

void
panel_refresh_ninfo(struct panel *p)
{
	if (selnode == NULL) {
		panel_set_content(p, "Select a node");
		return;
	}

	switch (st.st_mode) {
	case SM_JOBS:
		switch (selnode->n_state) {
		case JST_USED:
			panel_set_content(p,
			    "Node ID: %d\n"
			    "Login node ID: %d\n"
			    "Owner: %d\n"
			    "Job name: [%s]\n"
			    "Duration: %d\n"
			    "CPUs: %d",
			    selnode->n_nid,
			    selnode->n_logid,
			    selnode->n_job->j_owner,
			    selnode->n_job->j_name,
			    selnode->n_job->j_dur,
			    selnode->n_job->j_cpus);
			break;
		default:
			panel_set_content(p,
			    "Node ID: %d\n"
			    "Login node ID: %d",
			    selnode->n_nid,
			    selnode->n_logid);
			break;
		}
	}
}

void
panel_refresh_flyby(struct panel *p)
{
	if (active_flyby)
		panel_set_content(p, "Playing flyby");
	else if (build_flyby)
		panel_set_content(p, "Recording flyby");
	else
		panel_set_content(p, "Flyby mode disabled");
}

void
panel_refresh_goto(struct panel *p)
{
	if (goto_logid == -1)
		panel_set_content(p, "Login Node ID: %s",
		    buf_get(&uinp.uinp_buf));
	else
		panel_set_content(p, "Login Node ID: %d\n"
		    "Node ID: %s",
		    goto_logid,
		    buf_get(&uinp.uinp_buf));
}

void
draw_panels(void)
{
	struct panel *p, *np;

	/*
	 * Can't use TAILQ_FOREACH() since draw_panel() may remove
	 * a panel.
	 */
	panel_offset = 1;
	for (p = TAILQ_FIRST(&panels); p != TAILQ_END(&panels); p = np) {
		np = TAILQ_NEXT(p, p_link);
		p->p_refresh(p);
		draw_panel(p);
	}
}

void
panel_toggle(int panel)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link) {
		if (p->p_id == panel) {
			/* Found; toggle existence. */
			p->p_opts ^= POPT_REMOVE;
			fb.fb_panels |= panel;
			return;
		}
	}
	/* Not found; add. */
	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	memset(p, 0, sizeof(*p));
	p->p_str = NULL;
	p->p_strlen = 0;
	p->p_refresh = refreshf[baseconv(panel) - 1];
	p->p_id = panel;
	p->p_u = win_width;
	p->p_v = win_height - panel_offset - 1;
	p->p_fill.f_r = 1.0f;
	p->p_fill.f_g = 1.0f;
	p->p_fill.f_b = 1.0f;
	p->p_fill.f_a = 1.0f;
	fb.fb_panels |= panel;
	SLIST_INIT(&p->p_widgets);
	p->p_nwidgets = 0;
	TAILQ_INSERT_TAIL(&panels, p, p_link);
}

void
uinpcb_cmd(void)
{
	/* Send command to node. */
}

void
uinpcb_goto(void)
{
	size_t j;
	long l;

	l = strtol(buf_get(&uinp.uinp_buf), NULL, 0);
	if (!isdigit(*buf_get(&uinp.uinp_buf)) || l < 0 || l > NID_MAX)
		goto done;
	goto_logid = (int)l;

	for (j = 0; j < NLOGIDS; j++)
		if (logids[j] == goto_logid) {
			glutKeyboardFunc(keyh_uinput);
			uinp.uinp_callback = uinpcb_goto2;
			uinp.uinp_panel = PANEL_GOTO;
			return;
		}
done:
	panel_toggle(PANEL_GOTO);
}

void
uinpcb_goto2(void)
{
	struct node *n;
	int nid;
	long l;

	l = strtol(buf_get(&uinp.uinp_buf), NULL, 0);
	if (l <= 0 || l > NID_MAX)
		return;
	nid = (int)l;
	n = invmap[goto_logid][nid];

	if (n == NULL)
		return;
	if (st.st_opts & OP_TWEEN) {
		tx = n->n_pos.np_x;
		ty = n->n_pos.np_y;
		tz = n->n_pos.np_z;
	} else {
		st.st_x = n->n_pos.np_x;
		st.st_y = n->n_pos.np_y;
		st.st_z = n->n_pos.np_z;
	}
}
