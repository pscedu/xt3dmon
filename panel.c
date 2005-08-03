/* $Id$ */

/*
 * General notes for the panel-handling code.
 *
 *	(*) Panels are redrawn when the POPT_DIRTY flag is set in their
 *	    structure.  This means only that GL needs to re-render
 *	    the panel.  Panels must determine on their on, in their
 *	    refresh function, when their information itself is
 *	    "dirty."
 */

#include "compat.h"

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
void panel_refresh_ss(struct panel *);
void panel_refresh_status(struct panel *);
void panel_refresh_mem(struct panel *);

void uinpcb_ss(void);

struct pinfo pinfo[] = {
	{ panel_refresh_fps,	0,	 0,		 NULL },
	{ panel_refresh_ninfo,	0,	 0,		 NULL },
	{ panel_refresh_cmd,	PF_UINP, UINPO_LINGER,	 uinpcb_cmd },
	{ panel_refresh_legend,	0,	 0,		 NULL },
	{ panel_refresh_flyby,	0,	 0,		 NULL },
	{ panel_refresh_goto,	PF_UINP, 0,		 uinpcb_goto },
	{ panel_refresh_pos,	0,	 0,		 NULL },
	{ panel_refresh_ss,	PF_UINP, 0,		 uinpcb_ss },
	{ panel_refresh_status,	0,	 0,		 NULL },
	{ panel_refresh_mem,	0,	 0,		 NULL }
};

struct panels	 panels;
int		 panel_offset;
int		 mode_data_clean;
int		 selnode_clean;

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

	if (p->p_dl)
		glDeleteLists(p->p_dl, 1);
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
	int compile, npw, tweenadj, u, v, w, h;
	int lineno, curlen, toff, uoff, voff;
	struct pwidget *pw;
	char *s;

	if ((p->p_opts & POPT_DIRTY) == 0 && p->p_dl) {
		glCallList(p->p_dl);
		goto done;
	}

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
	if (p->p_opts & POPT_MOBILE) {
		u = p->p_u;
		v = p->p_v;
	}

	tweenadj = MAX(abs(p->p_u - u), abs(p->p_v - v));
	tweenadj = MAX(tweenadj, abs(p->p_w - w));
	tweenadj = MAX(tweenadj, abs(p->p_h - h));
	if (tweenadj) {
		tweenadj = sqrt(tweenadj);
		if (!tweenadj)
			tweenadj = 1;
		/*
		 * The panel is being animated, so don't time
		 * recompiling it.
		 */
		compile = 0;
	} else
		compile = 1;

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

	if (compile) {
		if (p->p_dl)
			glDeleteLists(p->p_dl, 1);
		p->p_dl = glGenLists(1);
		glNewList(p->p_dl, GL_COMPILE);
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

		uoff += p->p_w / 2 * (npw % 2 ? 1 : -1);
		if (npw % 2 == 0)
			voff -= PWIDGET_HEIGHT + PWIDGET_PADDING;
		if (voff - PWIDGET_HEIGHT < p->p_v - p->p_h)
			break;

		/* Draw widget background. */
		glBegin(GL_POLYGON);
		glColor4f(fp->f_r, fp->f_g, fp->f_b, 1.0f /* XXX */);
		glVertex2d(uoff + 1,			voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff - PWIDGET_HEIGHT + 1);
		glVertex2d(uoff + 1,			voff - PWIDGET_HEIGHT + 1);
		glEnd();

		/* Draw widget border. */
		glLineWidth(1.0f);
		glBegin(GL_LINE_LOOP);
		glColor4f(0.00f, 0.00f, 0.00f, 1.00f);
		glVertex2d(uoff,			voff);
		glVertex2d(uoff + PWIDGET_LENGTH + 1,	voff);
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

	glPushName(mkglname(p->p_id, GNAMT_PANEL));
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Draw background. */
	glBegin(GL_POLYGON);
	glColor4f(0.4, 0.6, 0.8, 0.8);
	glVertex2d(p->p_u,		p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
	glVertex2d(p->p_u,		p->p_v - p->p_h);
	glEnd();

	glDisable(GL_BLEND);

	/* Draw border. */
	glLineWidth(PANEL_BWIDTH);
	glBegin(GL_LINE_LOOP);
	glColor4f(0.2, 0.4, 0.6, 1.0);
	glVertex2d(p->p_u,		p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
	glVertex2d(p->p_u,		p->p_v - p->p_h);
	glEnd();
	glPopName();

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	if (compile) {
		glEndList();
		p->p_opts &= ~POPT_DIRTY;
		glCallList(p->p_dl);
	}
done:
	panel_offset += p->p_h + 3; /* spacing */
}

void
panel_set_content(struct panel *p, char *fmt, ...)
{
	struct panel *t;
	va_list ap;
	size_t len;

	len = 0; /* gcc */
	p->p_opts |= POPT_DIRTY;
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
	if (p->p_str[len - 1] == '\n')
		p->p_str[len - 1] = '\0';
	/* All panels below us must be refreshed now, too. */
	for (t = p; t != TAILQ_END(&panels); t = TAILQ_NEXT(t, p_link))
		t->p_opts |= POPT_DIRTY;
}

__inline int
panel_ready(struct panel *p)
{
	return ((p->p_opts & POPT_DIRTY) == 0 && p->p_str != NULL);
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
panel_refresh_fps(struct panel *p)
{
	static long ofps = -1;

	if (ofps == fps && panel_ready(p))
		return;
	ofps = fps;
	panel_set_content(p, "FPS: %d", fps);
}

void
panel_refresh_cmd(struct panel *p)
{
	/* XXX:  is the mode_data_clean check correct? */
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 &&
	    selnode_clean & PANEL_CMD && (SLIST_EMPTY(&selnodes) ||
	    (mode_data_clean & PANEL_CMD) == 0) && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;
	mode_data_clean |= PANEL_CMD;
	selnode_clean |= PANEL_CMD;

	if (SLIST_EMPTY(&selnodes))
		panel_set_content(p, "Please select a node\nto send a command to.");
	else
		panel_set_content(p, "Sending command to host\n\n> %s",
		    buf_get(&uinp.uinp_buf));
}

void
panel_refresh_legend(struct panel *p)
{
	struct pwidget *pw, *nextp;
	size_t j;

	if ((mode_data_clean & PANEL_LEGEND) && panel_ready(p))
		return;
	mode_data_clean |= PANEL_LEGEND;

	p->p_nwidgets = 0;
	p->p_maxwlen = 0;
	pw = SLIST_FIRST(&p->p_widgets);
	switch (st.st_mode) {
	case SM_JOBS:
		panel_set_content(p, "Jobs: %d", job_list.ol_cur);
		for (j = 0; j < NJST; j++, pw = nextp) {
			if (j == JST_USED)
				continue;
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &jstates[j].js_fill;
			pw->pw_str = jstates[j].js_name;
			p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		}
		for (j = 0; j < job_list.ol_cur; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &job_list.ol_jobs[j]->j_fill;
			pw->pw_str = job_list.ol_jobs[j]->j_jname;
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
		for (j = 0; j < fail_list.ol_cur; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &fail_list.ol_fails[j]->f_fill;
			pw->pw_str = fail_list.ol_fails[j]->f_name;
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
		for (j = 0; j < TEMP_NTEMPS; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pw->pw_fillp = &temp_map[j].m_fill;
			pw->pw_str = temp_map[j].m_name;
			p->p_maxwlen = MAX(p->p_maxwlen, strlen(pw->pw_str));
		}
		break;
	}
}

void
panel_refresh_ninfo(struct panel *p)
{
	struct objhdr *ohp;
	struct objlist *ol;
	struct selnode *sn;
	struct node *n;

	if (selnode_clean & PANEL_NINFO && (SLIST_EMPTY(&selnodes) ||
	    (mode_data_clean & PANEL_NINFO)) && panel_ready(p))
		return;
	mode_data_clean |= PANEL_NINFO;
	selnode_clean |= PANEL_NINFO;

	if (SLIST_EMPTY(&selnodes)) {
		panel_set_content(p, "Please select some nodes");
		return;
	}

	if (nselnodes > 1) {
		char *label, nids[BUFSIZ], data[BUFSIZ];
		size_t j, nids_pos, data_pos;

		nids[0] = data[0] = '\0';
		nids_pos = data_pos = 0;

		SLIST_FOREACH(sn, &selnodes, sn_next) {
			n = sn->sn_nodep;
			nids_pos += snprintf(nids + nids_pos,
			    sizeof(nids) - nids_pos, ",%d", n->n_nid);
			if (nids_pos >= sizeof(nids))
				break;
			switch (st.st_mode) {
			case SM_JOBS:
				if (n->n_state == JST_USED)
					n->n_job->j_oh.oh_flags |= OHF_SEL;
				break;
			case SM_TEMP:
				break;
			case SM_FAIL:
				break;
			}
		}

		label = NULL; /* gcc */
		ol = NULL; /* gcc */
		switch (st.st_mode) {
		case SM_JOBS:
			label = "Job IDs";
			ol = &job_list;
			break;
		case SM_TEMP:
			label = "Temperatures";
			ol = &temp_list;
			break;
		case SM_FAIL:
			label = "Failures";
			ol = &fail_list;
			break;
		}

		for (j = 0; j < ol->ol_cur; j++) {
			ohp = ol->ol_data[j];
			if (ohp->oh_flags & OHF_SEL) {
				ohp->oh_flags &= ~OHF_SEL;
				switch (st.st_mode) {
				case SM_JOBS:
					data_pos += snprintf(data + data_pos,
					    sizeof(data) - data_pos, ",%d",
					    ((struct job *)ohp)->j_id);
					break;
				case SM_TEMP:
					break;
				case SM_FAIL:
					break;
				}
			}
			if (data_pos >= sizeof(data))
				break;
		}

		if (data[0] == '\0')
			strncpy(data, "_(none)", sizeof(data) - 1);

		panel_set_content(p,
		    "%d node(s) selected\n"
		    "Node IDs: %s\n"
		    "%s: %s",
		    nselnodes, nids + 1,
		    label, data + 1);
		return;
	}
	n = SLIST_FIRST(&selnodes)->sn_nodep;
	switch (st.st_mode) {
	case SM_JOBS:
		switch (n->n_state) {
		case JST_USED:
			panel_set_content(p,
			    "Node ID: %d\n"
			    "Job ID: %d\n"
			    "Job owner: %s\n"
			    "Job name: %s\n"
			    "Job queue: %s\n"
			    "Job duration: %d:%02d\n"
			    "Job time used: %d:%02d (%d%%)\n"
			    "Job CPUs: %d",
			    n->n_nid,
			    n->n_job->j_id,
			    n->n_job->j_owner,
			    n->n_job->j_jname,
			    n->n_job->j_queue,
			    n->n_job->j_tmdur / 60,
			    n->n_job->j_tmdur % 60,
			    n->n_job->j_tmuse / 60,
			    n->n_job->j_tmuse % 60,
			    n->n_job->j_tmuse * 100 /
			      (n->n_job->j_tmdur ?
			       n->n_job->j_tmdur : 1),
			    n->n_job->j_ncpus);
			break;
		default:
			panel_set_content(p,
			    "Node ID: %d\n"
			    "Type: %s",
			    n->n_nid,
			    jstates[n->n_state].js_name);
			break;
		}
		break;
	case SM_FAIL:
		panel_set_content(p,
		    "Node ID: %d\n"
		    "# Failures: %s",
		    n->n_nid,
		    n->n_fail->f_name);
		break;
	case SM_TEMP:
		panel_set_content(p,
		    "Node ID: %d\n"
		    "Temperature: %s",
		    n->n_nid,
		    n->n_temp->t_name);
		break;
	}
}

void
panel_refresh_flyby(struct panel *p)
{
	static int sav_mode = -1;

	if (sav_mode == flyby_mode && panel_ready(p))
		return;
	sav_mode = flyby_mode;

	switch (flyby_mode) {
	case FBM_PLAY:
		panel_set_content(p, "Playing flyby");
		break;
	case FBM_REC:
		panel_set_content(p, "Recording flyby");
		break;
	default:
		panel_set_content(p, "Flyby mode disabled");
		break;
	}
}

void
panel_refresh_goto(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "Node ID: %s", buf_get(&uinp.uinp_buf));
}

void
panel_refresh_mem(struct panel *p)
{
	if ((mode_data_clean & PANEL_MEM) && panel_ready(p))
		return;
	mode_data_clean |= PANEL_MEM;
	panel_set_content(p, "VSZ: %lu\nRSS: %ld\n", vmem, rmem);
}

void
panel_refresh_pos(struct panel *p)
{
	static struct fvec v, lv;

#if 0
		if (st.st_opts & OP_TWEEN &&
		    st.st_x == tx && st.st_lx == tlx &&
		    st.st_y == ty && st.st_ly == tly &&
		    st.st_z == tz && st.st_lz == tlz)
			/* don't compile */;
		else
#endif

	if (v.fv_x == st.st_x && lv.fv_x == st.st_lx &&
	    v.fv_y == st.st_y && lv.fv_y == st.st_ly &&
	    v.fv_z == st.st_z && lv.fv_z == st.st_lz &&
	    panel_ready(p))
		return;

	v.fv_x = st.st_x;
	v.fv_y = st.st_y;
	v.fv_z = st.st_z;

	lv.fv_x = st.st_lx;
	lv.fv_y = st.st_ly;
	lv.fv_z = st.st_lz;

	panel_set_content(p, "Position (%.2f,%.2f,%.2f)\n"
	    "Look (%.2f,%.2f,%.2f)", st.st_x, st.st_y, st.st_z,
	    st.st_lx, st.st_ly, st.st_lz);
}

void
panel_refresh_ss(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "Screenshot filename: %s",
	    buf_get(&uinp.uinp_buf));
}

int panel_status_dirty;
char panel_status_content[BUFSIZ];

void
panel_status_setinfo(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(panel_status_content, sizeof(panel_status_content),
	    fmt, ap);
	va_end(ap);

	panel_status_dirty = 1;
}

void
panel_status_addinfo(const char *fmt, ...)
{
	va_list ap;
	int len;

	len = strlen(panel_status_content);

	va_start(ap, fmt);
	vsnprintf(panel_status_content + len,
	    sizeof(panel_status_content) - len, fmt, ap);
	va_end(ap);

	panel_status_dirty = 1;
}

void
panel_refresh_status(struct panel *p)
{
	if (panel_status_dirty == 0 && panel_ready(p))
		return;
	panel_status_dirty = 0;
	panel_set_content(p, "Status\n%s", panel_status_content);
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

struct panel *
panel_for_id(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id)
			return (p);
	return (NULL);
}

void
panel_show(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id) {
			if (p->p_opts & POPT_REMOVE)
				panel_tremove(p);
			return;
		}
	panel_toggle(id);
}

void
panel_hide(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id) {
			if ((p->p_opts & POPT_REMOVE) == 0)
				panel_tremove(p);
			return;
		}
}

void
panel_tremove(struct panel *p)
{
	struct panel *t;

	/* XXX inspect POPT_REMOVE first? */
	p->p_opts ^= POPT_REMOVE;
	for (t = p; t != TAILQ_END(&panels); t = TAILQ_NEXT(t, p_link))
		t->p_opts |= POPT_DIRTY;
	if (flyby_mode == FBM_REC)
		flyby_writepanel(p->p_id);
}

void
panel_toggle(int panel)
{
	struct pinfo *pi;
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == panel) {
			panel_tremove(p);
			return;
		}
	/* Not found; add. */
	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	memset(p, 0, sizeof(*p));

	pi = &pinfo[baseconv(panel) - 1];

	p->p_str = NULL;
	p->p_refresh = pi->pi_refresh;
	p->p_id = panel;
	p->p_u = win_width;
	p->p_v = win_height - panel_offset - 1;
	p->p_fill.f_r = 1.0f;
	p->p_fill.f_g = 1.0f;
	p->p_fill.f_b = 1.0f;
	p->p_fill.f_a = 1.0f;
	SLIST_INIT(&p->p_widgets);

	if (pi->pi_opts & PF_UINP) {
		glutKeyboardFunc(keyh_uinput);
		uinp.uinp_panel = p;
		uinp.uinp_opts = pi->pi_uinpopts;
		uinp.uinp_callback = pi->pi_uinpcb;
	}
	TAILQ_INSERT_TAIL(&panels, p, p_link);
	if (flyby_mode == FBM_REC)
		flyby_writepanel(p->p_id);
}

void
flip_panels(int panels)
{
	int b;

	while (panels) {
		b = ffs(panels) - 1;
		panels &= ~(1 << b);
		panel_toggle(1 << b);
	}
}

/*
 * Set panels state to those specified in `start.'
 */
void
init_panels(int start)
{
	struct panel *p;
	int cur;

	cur = 0;
	TAILQ_FOREACH(p, &panels, p_link)
		cur |= p->p_id;
	flip_panels((cur ^ start) & ~FB_PMASK);
}
