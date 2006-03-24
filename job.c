/* $Id$ */

#include "mon.h"

#include "cdefs.h"
#include "job.h"

struct job *
job_findbyid(int id, int *pos)
{
	size_t n;

	for (n = 0; n < job_list.ol_cur; n++)
		if (job_list.ol_jobs[n]->j_id == id) {
			if (pos)
				*pos = n;
			return (job_list.ol_jobs[n]);
		}
	if (pos)
		*pos = -1;
	return (NULL);
}

#if 0
struct job *
job_findbyid(size_t id)
{
	size_t n, tid;
	int lo, hi;

	lo = 0;
	hi = MAX(job_list.ol_tcur, job_list.ol_cur);
	if (hi == 0)
		return (NULL);
	while (lo <= hi) {
		n = MID(lo, hi);
		tid = job_list.ol_jobs[n]->j_id;
		if (tid < id)
			lo = n + 1;
		else if (tid > id)
			hi = n - 1;
		else
			return (job_list.ol_jobs[n]);
	}
	return (NULL);
}
#endif

__inline void
job_hl(struct job *j)
{
	j->j_fill.f_a = 1.0f;
}

void
job_goto(__unused struct job *j)
{
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

	for (r = 0; r < NROWS; r++) {
		for (cb = 0; cb < NCABS; cb++) {

			memset(jobstats, '\0', siz);

			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
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

//			glPushMatrix();
			vp = nodes[r][cb][2][0][0].n_v;
			fp = &jp->j_fill;
			glColor3f(fp->f_r, fp->f_g, fp->f_b);
			glRasterPos3d(vp->fv_x + CABWIDTH / 2.0,
			    vp->fv_y + CAGEHEIGHT + 1.0,
			    vp->fv_z + ROWDEPTH / 2.0);
			for (p = buf; *p != '\0'; p++)
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
//			glPopMatrix();
		}
	}

	free(jobstats);
}

#endif
