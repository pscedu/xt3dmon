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
#include <math.h>
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
void panel_refresh_pos(struct panel *);

struct pinfo pinfo[] = {
	{ panel_refresh_fps,	0,	 0,		 NULL },
	{ panel_refresh_ninfo,	0,	 0,		 NULL },
	{ panel_refresh_cmd,	PF_UINP, UINPO_LINGER,	 uinpcb_cmd },
	{ panel_refresh_legend,	0,	 0,		 NULL },
	{ panel_refresh_flyby,	0,	 0,		 NULL },
	{ panel_refresh_goto,	PF_UINP, 0,		 uinpcb_goto },
	{ panel_refresh_pos,	0,	 0,		 NULL }
};

struct panels	 panels;
int		 panel_offset;

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
	int lineno, curlen, toff, uoff, voff, npw, tweenadj, u, v, w, h;
	struct pwidget *pw;
	char *s;

	toff = PANEL_PADDING + PANEL_BWIDTH;

	if (p->p_opts & POPT_REMOVE) {
		w = 0;
		h = 0;
		u = win_width;
		v = win_height - panel_offset;
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

	tweenadj = MAX(abs(p->p_u - u), abs(p->p_v - v));
	tweenadj = MAX(tweenadj, abs(p->p_w - w));
	tweenadj = MAX(tweenadj, abs(p->p_h - h));
	if (tweenadj) {
		tweenadj = sqrt(tweenadj);
		if (!tweenadj)
			tweenadj = 1;
	}

	if (p->p_u != u)
		p->p_u -= (p->p_u - u) / tweenadj;
	if (p->p_v != v)
		p->p_v -= (p->p_v - v) / tweenadj;
	if (p->p_w != w)
		p->p_w -= (p->p_w - w) / tweenadj;
	if (p->p_h != h)
		p->p_h -= (p->p_h - h) / tweenadj;

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
			    "Owner: %d\n"
			    "Job name: [%s]\n"
			    "Duration: %d\n"
			    "CPUs: %d",
			    selnode->n_nid,
			    selnode->n_job->j_owner,
			    selnode->n_job->j_name,
			    selnode->n_job->j_dur,
			    selnode->n_job->j_cpus);
			break;
		default:
			panel_set_content(p,
			    "Node ID: %d\n"
			    "Type: %s",
			    selnode->n_nid,
			    jstates[selnode->n_state].js_name);
			break;
		}
		break;
	case SM_FAIL:
		panel_set_content(p,
		    "Node ID: %d\n"
		    "# Failures: %s",
		    selnode->n_nid,
		    selnode->n_fail->f_name);
		break;
	case SM_TEMP:
		panel_set_content(p,
		    "Node ID: %d\n"
		    "Temperature: %s",
		    selnode->n_nid,
		    selnode->n_temp->t_name);
		break;
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
	panel_set_content(p, "Node ID: %s", buf_get(&uinp.uinp_buf));
}

void
panel_refresh_pos(struct panel *p)
{
	panel_set_content(p, "Position (%.2f,%.2f,%.2f)\n"
	    "Look (%.2f,%.2f,%.2f)", st.st_x, st.st_y, st.st_z,
	    st.st_lx, st.st_ly, st.st_lz);
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
	struct pinfo *pi;
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

	pi = &pinfo[baseconv(panel) - 1];

	p->p_str = NULL;
	p->p_strlen = 0;
	p->p_refresh = pi->pi_refresh;
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

	if (pi->pi_opts & PF_UINP) {
		glutKeyboardFunc(keyh_uinput);
		uinp.uinp_panel = panel;
		uinp.uinp_opts = pi->pi_uinpopts;
		uinp.uinp_callback = pi->pi_uinpcb;
	}
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
	struct node *n;
	char *s;
	int nid;
	long l;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 0);
	if (l <= 0 || l > NID_MAX || !isdigit(*s))
		return;
	nid = (int)l;

	if ((n = node_for_nid(nid)) == NULL)
		return;
	if (st.st_opts & OP_TWEEN) {
		tx = n->n_v->v_x;
		ty = n->n_v->v_y;
		tz = n->n_v->v_z;
	} else {
		st.st_x = n->n_v->v_x;
		st.st_y = n->n_v->v_y;
		st.st_z = n->n_v->v_z;
	}
}
