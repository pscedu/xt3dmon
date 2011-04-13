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

#include "cdefs.h"
#include "job.h"
#include "util.h"

#define JINCR	10

int job_eq(const void *, const void *);

struct objlist job_list = { NULL, 0, 0, 0, 0, JINCR, sizeof(struct job), job_eq };

int
job_eq(const void *elem, const void *arg)
{
	return (((struct job *)elem)->j_id == *(int *)arg);
}

int
job_cmp(const void *a, const void *b)
{
	return (CMP((*(struct job **)a)->j_id, (*(struct job **)b)->j_id));
}

struct job *
job_findbyid(int id, int *pos)
{
	size_t n;

	for (n = 0; n < job_list.ol_cur; n++)
		if (OLE(job_list, n, job)->j_id == id) {
			if (pos)
				*pos = n;
			return (OLE(job_list, n, job));
		}
	if (pos)
		*pos = -1;
	return (NULL);
}

__inline void
job_hl(struct job *j)
{
	j->j_fill.f_a = 1.0f;
}

#if 0
int
jscmpf(const void *a, const void *b)
{
	if (*(int *)a < *(int *)b)
		return (-1);
	else if (*(int *)a > *(int *)b)
		return (1);
	else
		return (0);
}

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "node.h"
#include "state.h"

void
job_drawlabels(void)
{
	int r, cb, cg, m, n, *jobstats;
	size_t j, i, siz, jmax, jmaxstat;
	char *p, buf[BUFSIZ];
	struct fvec *vp;
	struct node *np;
	struct fill *fp;
	struct job *jp;

	siz = job_list.ol_cur * sizeof(*jobstats);
	if ((jobstats = malloc(siz)) == NULL)
		err(1, "malloc");

	glPushMatrix();

	for (r = 0; r < NROWS; r++) {
		for (cb = 0; cb < NRACKS; cb++) {

			memset(jobstats, '\0', siz);

			for (cg = 0; cg < NIRUS; cg++)
				for (m = 0; m < NBLADES; m++)
					for (n = 0; n < NNODES; n++) {
						np = &nodes[r][cb][cg][m][n];
						if (np->n_job) {
							for (j = 0; j < job_list.ol_cur; j++)
								if (job_list.ol_jobs[j] == np->n_job) {
									jobstats[j]++;
									break;
								}
#if 0
							if (j == job_list.ol_cur)
								errx(1, "job not found");
#endif
						}
					}

			jmax = 0;
			jmaxstat = 0;
			for (j = 0; j < job_list.ol_cur; j++)
				if (jobstats[j] > jmaxstat) {
					jmax = j;
					jmaxstat = jobstats[j];
				}
			jp = job_list.ol_jobs[jmax];
			snprintf(buf, sizeof(buf), "job %d", jp->j_id);

			vp = nodes[r][cb][2][0][0].n_v;
			fp = &jp->j_fill;
			glColor3f(fp->f_r, fp->f_g, fp->f_b);
			glRasterPos3d(vp->fv_x + CABWIDTH / 2.0,
			    vp->fv_y + CAGEHEIGHT * 2.0,
			    vp->fv_z + ROWDEPTH / 2.0);
			for (p = buf; *p != '\0'; p++)
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
		}
	}
	glPopMatrix();

	free(jobstats);
}
#endif
