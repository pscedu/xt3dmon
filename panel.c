/* $Id$ */

#include <sys/types.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mon.h"
#include "queue.h"
#include "buf.h"

#define LETTER_HEIGHT 13
#define LETTER_WIDTH 8
#define PANEL_PADDING 3
#define PANEL_BWIDTH 2.0f

void panel_refresh_fps(struct panel *);
void panel_refresh_ninfo(struct panel *);
void panel_refresh_cmd(struct panel *);
void panel_refresh_flegend(struct panel *);
void panel_refresh_jlegend(struct panel *);

void (*refreshf[])(struct panel *) = {
	panel_refresh_fps,
	panel_refresh_ninfo,
	panel_refresh_cmd,
	panel_refresh_flegend,
	panel_refresh_jlegend
};

struct panels panels;
int panel_offset;

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
draw_panel(struct panel *p)
{
	int lineno, curlen, line_offset, toff;
	char *s;

#if 0
	if (p->p_adju) {
		p->p_su += p->p_adju;
		if (p->p_su <= 0) {
printf("removing\n");
			TAILQ_REMOVE(p, p_link);
			panel_offset -= p->p_h;
			free(p->p_str);
			free(p);
			return (0.0f);
		} else if (p->p_su >= p->p_u)
{printf("moved into place\n");
			p->p_adju = 0;
}
	}
	if (p->p_adjv) {
		p->p_sv += p->p_adjv;
//		if (p->p_sv >= )
	}
#endif

	/* Save state and set things up for 2D. */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, win_width, 0.0, win_height);

	glColor4f(p->p_r, p->p_g, p->p_b, p->p_a);

	toff = PANEL_PADDING + PANEL_BWIDTH;

	lineno = 1;
	curlen = p->p_w = 0;
	for (s = p->p_str; *s != '\0'; s++, curlen++) {
		if (*s == '\n') {
			if (curlen > p->p_w)
				p->p_w = curlen;
			curlen = -1;
			lineno++;
		}
	}
	if (curlen > p->p_w)
		p->p_w = curlen;
//printf("offset: %d\n", offset);
	p->p_w = p->p_w * LETTER_WIDTH + 2 * toff;
	p->p_h = lineno * LETTER_HEIGHT + 2 * toff;
	p->p_u = win_width - p->p_w;
	p->p_v = win_height - panel_offset;

	if (p->p_su != p->p_u)
		p->p_su += p->p_su - p->p_u < 0 ? 1 : -1;
	if (p->p_sv != p->p_v)
		p->p_sv += p->p_sv - p->p_v < 0 ? 1 : -1;

//printf("write '%s'\n", p->p_str);
	/* Panel content. */
	line_offset = p->p_sv - toff - LETTER_HEIGHT;
	glRasterPos2d(p->p_su + toff, line_offset);
	for (s = p->p_str; *s != '\0'; s++) {
		if (*s == '\n') {
			line_offset -= LETTER_HEIGHT;
			glRasterPos2d(p->p_su + toff, line_offset);
		} else
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);
	}

	/* Draw background. */
	glBegin(GL_POLYGON);
	glColor4f(0.20, 0.40, 0.5, 1.0);
	glVertex2d(p->p_su,		p->p_sv);
	glVertex2d(p->p_su + p->p_w,	p->p_sv);
	glVertex2d(p->p_su + p->p_w,	p->p_sv - p->p_h);
	glVertex2d(p->p_su,		p->p_sv - p->p_h);
	glEnd();

	/* Draw border. */
	glLineWidth(PANEL_BWIDTH);
	glBegin(GL_LINE_LOOP);
	glColor4f(0.40, 0.80, 1.0, 1.0);
	glVertex2d(p->p_su,		p->p_sv);
	glVertex2d(p->p_su + p->p_w,	p->p_sv);
	glVertex2d(p->p_su + p->p_w,	p->p_sv - p->p_h);
	glVertex2d(p->p_su,		p->p_sv - p->p_h);
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
printf("resizing %d to %d\n", p->p_strlen, len);
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
	if (selnode)
		panel_set_content(p, "Sending command to host\n\n> %s", 
		    buf_get(&cmdbuf));
	else
		panel_set_content(p, "Please select a node\nto send a command to.");
}

void
panel_refresh_jlegend(struct panel *p)
{
	panel_set_content(p, "Jobs: %d", njobs);
}

void
panel_refresh_flegend(struct panel *p)
{
	panel_set_content(p, "flegend");
}

void
panel_refresh_ninfo(struct panel *p)
{
	if (selnode) {
		switch (selnode->n_savst) {
		case ST_USED:
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
			    "Node ID: %d",
			    selnode->n_nid);
			break;
		}
	} else {
		panel_set_content(p, "Select a node");
	}
}

void
adjpanels(void)
{
	struct panel *p;

	/* Resize happening -- move panels. */
	float v = win_height;
	TAILQ_FOREACH(p, &panels, p_link) {
		if (p->p_su < win_width - p->p_w)
			p->p_adju = 1;
		else if (p->p_su > win_width - p->p_w)
			p->p_adju = -1;
		p->p_su = win_width - p->p_w;

		if (p->p_sv < v)
			p->p_adjv = 1;
		else if (p->p_sv < v)
			p->p_adjv = -1;
		p->p_sv = v;

		v -= p->p_h;
	}
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
			/* Found; make it disappear. */
			p->p_u = win_width;
			return;
		}
	}
	/* Not found; add. */
	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	p->p_adju = 1;
	p->p_str = NULL;
	p->p_strlen = 0;
printf("panel id %d, index %d\n", panel, baseconv(panel) - 1);
	p->p_refresh = refreshf[baseconv(panel) - 1];
	p->p_id = panel;
	p->p_su = win_width;
printf("showing panel with offset %d\n", panel_offset);
	p->p_sv = win_height - panel_offset - 1;
	p->p_r = 1.0f;
	p->p_g = 1.0f;
	p->p_b = 1.0f;
	p->p_a = 1.0f;
	TAILQ_INSERT_TAIL(&panels, p, p_link);
}
