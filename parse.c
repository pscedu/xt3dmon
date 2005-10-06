/* $Id$ */

#include "compat.h"

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "mon.h"

#define MAXFAILS	200

int		 total_failures;
unsigned long	 vmem;
long		 rmem;

/*
 * Example line:
 *	33     c0-0c1s0s1      0,5,0
 *
 * Broken up:
 *	nid	cb r cb  m  n	x y z
 *	33	c0-0 c1 s0 s1	0,5,0
 */
void
parse_physmap(int *fd)
{
	int lineno, r, cb, cg, m, n, nid, x, y, z;
	char buf[BUFSIZ], *p, *s;
	struct node *node;
	struct ivec widim;
	FILE *fp;
	long l;

	widim.iv_w = widim.iv_h = widim.iv_d = 0;

	/* Explicitly initialize all nodes. */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_state = JST_UNACC;
						node->n_fillp = &jstates[JST_UNACC].js_fill;
						node->n_flags |= NF_HIDE;
					}

	if ((fp = fdopen(*fd, "r")) == NULL) {
		warn("fdopen");
		return;
	}
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;

		/* nid */
		if (!isdigit(*p))
			goto bad;
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NID_MAX)
			goto bad;
		nid = (int)l;

		/* cb */
		p = s;
		while (isspace(*p))
			p++;
		if (*p++ != 'c')
			goto bad;
		if (!isdigit(*p))
			goto bad;
		s = p;
		while (isdigit(*++s))
			;
		if (*s != '-')
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NCABS)
			goto bad;
		cb = (int)l;

		/* r */
		p = s + 1;
		s = p;
		while (isdigit(*++s))
			;
		if (*s != 'c')
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NROWS)
			goto bad;
		r = (int)l;

		/* cg */
		p = s + 1;
		s = p;
		while (isdigit(*++s))
			;
		if (*s != 's')
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NCAGES)
			goto bad;
		cg = (int)l;

		/* m */
		p = s;
		while (isdigit(*++s))
			;
		if (*s != 's')
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NMODS)
			goto bad;
		m = (int)l;

		/* n */
		p = s;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NNODES)
			goto bad;
		n = (int)l;

		/* x */
		while (isspace(*s))
			s++;
		if (!isdigit(*s))
			goto bad;
		p = s;
		while (isdigit(*++s))
			;
		if (*s != ',')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= WIDIM_WIDTH)
			goto bad;
		x = (int)l;

		/* y */
		while (isspace(*s))
			s++;
		if (!isdigit(*s))
			goto bad;
		p = s;
		while (isdigit(*++s))
			;
		if (*s != ',')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= WIDIM_HEIGHT)
			goto bad;
		y = (int)l;

		/* z */
		while (isspace(*s))
			s++;
		if (!isdigit(*s))
			goto bad;
		p = s;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= WIDIM_DEPTH)
			goto bad;
		z = (int)l;

		/* done */

		node = &nodes[r][cb][cg][m][n];
		node->n_nid = nid;
		invmap[nid] = node;
		wimap[x][y][z] = node;
		node->n_wiv.iv_x = x;
		node->n_wiv.iv_y = y;
		node->n_wiv.iv_z = z;

		if (x > widim.iv_w)
			widim.iv_w = x;
		if (y > widim.iv_h)
			widim.iv_h = y;
		if (z > widim.iv_d)
			widim.iv_d = z;

		/* state */
		while (isspace(*s))
			s++;
		if (!isalpha(*s))
			goto bad;
		switch (*s) {
		case 'c': /* compute */
			node->n_state = JST_FREE;	/* Good enough. */
			break;
		case 'n': /* disabled */
			node->n_state = JST_DISABLED;
			break;
		case 'i': /* I/O */
			node->n_state = JST_IO;
			break;
		default:
			goto bad;
		}
		node->n_fillp = &jstates[node->n_state].js_fill;
		node->n_flags &= ~NF_HIDE;
		continue;
bad:
		warnx("physmap:%d: malformed line [%s] [%s]", lineno,
		    buf, p);
	}
	if (ferror(fp))
		warn("fgets");
	fclose(fp);
	*fd = -1;
	errno = 0;

	if (++widim.iv_w != WIDIM_WIDTH ||
	    ++widim.iv_h != WIDIM_HEIGHT ||
	    ++widim.iv_d != WIDIM_DEPTH)
		errx(1, "wired cluster dimensions have changed");
}

/*
 * Example:
 *	40 1 3661
 *
 * Broken down:
 *	nid enabled jobid
 *	40  1       3661
 *
 * Bad/check format:
 *	nid disabled (if non-zero)
 *	40  1
 */
void
parse_jobmap(int *fd)
{
	int jobid, nid, lineno, enabled;
	char buf[BUFSIZ], *p, *s;
	struct node *node;
	struct job *job;
	size_t j;
	FILE *fp;
	long l;

	/* XXXXXX - reset fillp on all nodes. */

	if ((fp = fdopen(*fd, "r")) == NULL) {
		warn("fdopen");
		return;
	}
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;
		if (!isdigit(*p))
			goto bad;

		/* nid */
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
			goto bad;
		nid = (int)l;

		/* enabled */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		enabled = (int)l;

		/* job id */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		jobid = (int)l;

		if ((node = node_for_nid(nid)) == NULL) {
			if (enabled)
				warnx("inconsistency: node %d should be "
				    "disabled in jobmap", nid);
			goto pass;
		}

		if (enabled == 0)
			node->n_state = JST_DOWN;
		else if (jobid == 0)
			node->n_state = JST_FREE;
		else {
			node->n_state = JST_USED;
			job = getobj(&jobid, &job_list);
			job->j_id = jobid;
			/* XXX: only slightly sloppy. */
			job->j_fill.f_texid = jstates[JST_USED].js_fill.f_texid;
			job->j_fill.f_texid_a = jstates[JST_USED].js_fill.f_texid_a;
			node->n_job = job;
			node->n_fillp = &job->j_fill;
		}

		if (node->n_state != JST_USED)
			node->n_fillp = &jstates[node->n_state].js_fill;
		continue;
bad:
		warn("jobmap:%d: malformed line", lineno);
pass:
		; //node->n_fillp = ;
	}
	if (ferror(fp))
		warn("fgets");
	fclose(fp);
	*fd = -1;

	ds_refresh(DS_BAD, DSF_IGN);
	ds_refresh(DS_CHECK, DSF_IGN);
	errno = 0;

	qsort(job_list.ol_jobs, job_list.ol_tcur, sizeof(struct job *),
	    job_cmp);
	for (j = 0; j < job_list.ol_tcur; j++)
		getcol(j, job_list.ol_tcur, &job_list.ol_jobs[j]->j_fill);

	/* Must read qstat info last so jobs are in our memory. */
	ds_refresh(DS_QSTAT, 0);
}

void
parse_badmap(int *fd)
{
	char *s, *p, buf[BUFSIZ];
	int lineno, bad, nid;
	struct node *n;
	FILE *fp;
	long l;

	if ((fp = fdopen(*fd, "r")) == NULL)
		return;						/* Failure is OK. */
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;
		if (!isdigit(*p))
			goto bad;

		/* nid */
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
			goto bad;
		nid = (int)l;

		/* disabled */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		bad = (int)l;

		if (bad) {
			if ((n = node_for_nid(nid)) == NULL)
				goto bad;
			n->n_state = JST_BAD;
			n->n_fillp = &jstates[JST_BAD].js_fill;
		}
		continue;
bad:
		warnx("badmap:%d: malformed line", lineno);
	}
	if (ferror(fp))
		warn("fgets");
	fclose(fp);
	*fd = -1;
}

void
parse_checkmap(int *fd)
{
	char *s, *p, buf[BUFSIZ];
	int lineno, checking, nid;
	struct node *n;
	FILE *fp;
	long l;

	if ((fp = fdopen(*fd, "r")) == NULL)
		return;						/* Failure is OK. */
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;
		if (!isdigit(*p))
			goto bad;

		/* nid */
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
			goto bad;
		nid = (int)l;

		/* disabled */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		checking = (int)l;

		if (checking) {
			if ((n = node_for_nid(nid)) == NULL)
				goto bad;
			n->n_state = JST_CHECK;
			n->n_fillp = &jstates[JST_BAD].js_fill;
		}
		continue;
bad:
		warnx("checkmap:%d: malformed line", lineno);
	}
	if (ferror(fp))
		warn("fgets");
	fclose(fp);
	*fd = -1;
}

/*
 * Parse failure data entries.
 *
 * Format:
 *	1		2724
 *	<nfailures>	<nid>
 *
 * Notes:
 *	zero data is not listed.
 */
void
parse_failmap(int *fd)
{
	int newmax, nofails, nid, lineno, r, cb, cg, m, n;
	char *p, *s, buf[BUFSIZ];
	struct node *node;
	struct fail *fail;
	size_t j;
	FILE *fp;
	long l;

	if ((fp = fdopen(*fd, "r")) == NULL) {
		warn("fdopen");
		return;
	}

	/*
	 * Because entries with zero failures are not listed,
	 * we must go through and reset all entries.
	 */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_fail = &fail_notfound;
						node->n_fillp = &fail_notfound.f_fill;
					}

	total_failures = lineno = 0;
	newmax = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		for (s = p; isdigit(*s); s++)
			;
		if (s == p || !isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= INT_MAX)
			goto bad;
		nofails = (int)l;

		p = s + 1;
		while (isspace(*p))
			p++;
		for (s = p; isdigit(*s); s++)
			;
		if (s == p)
			goto bad;
		*s = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NID_MAX)
			goto bad;
		nid = (int)l;

		if (nofails > MAXFAILS)
			nofails = MAXFAILS;

		if ((node = node_for_nid(nid)) == NULL)
			goto bad;
		fail = getobj(&nofails, &fail_list);
		fail->f_fails = nofails;
		free(fail->f_name);		/* XXX - rename to f_label */
		if (asprintf(&fail->f_name, "%d", nofails) == -1)
			err(1, "asprintf");
		node->n_fillp = &fail->f_fill;
		node->n_fail = fail;

		/* Compute failure statistics. */
		total_failures += nofails;
		if (nofails > newmax)
			newmax = nofails;
		continue;
bad:
		warnx("%s:%d: malformed line: %s [s: %s, p: %s]",
		    _PATH_FAILMAP, lineno, buf, s, p);
	}
	if (ferror(fp))
		warn("%s", _PATH_FAILMAP);
	fclose(fp);
	*fd = -1;
	errno = 0;

	qsort(fail_list.ol_fails, fail_list.ol_tcur, sizeof(struct fail *),
	    fail_cmp);
	for (j = 0; j < fail_list.ol_tcur; j++)
		getcol(j, fail_list.ol_tcur, &fail_list.ol_fails[j]->f_fill);
}

/*
 * Temperature data.
 *
 * Example:
 *	cx0y0c0s4	    20  18  18  19
 *	position	[[[[t1] t2] t3] t4]
 */
void
parse_tempmap(int *fd)
{
	int t, lineno, i, r, cb, cg, m, n;
	char buf[BUFSIZ], *p, *s;
	struct node *node;
	struct temp *temp;
	size_t j;
	FILE *fp;
	long l;

	if ((fp = fdopen(*fd, "r")) == NULL) {
		warn("fdopen");
		return;
	}

	/*
	 * We're not guarenteed to have temperature information for
	 * every node...
	 */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_temp = &temp_notfound;
						node->n_fillp = &temp_notfound.t_fill;
					}

	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;

		while (isspace(*p))
			p++;
		if (*p == '#' || *p == '\n' || *p == '\0')
			continue;
		if (*p++ != 'c')
			goto bad;
		if (*p++ != 'x')
			goto bad;

		/* cab */
		if (!isdigit(*p))
			goto bad;
		for (s = p + 1; isdigit(*s); s++)
			;
		if (*s != 'y')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NCABS)
			goto bad;
		cb = (int)l;

		/* row */
		p = s;
		while (isdigit(*s))
			s++;
		if (p == s)
			goto bad;
		if (*s != 'c')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NROWS)
			goto bad;
		r = (int)l;

		/* cage */
		p = s;
		while (isdigit(*s))
			s++;
		if (p == s)
			goto bad;
		if (*s != 's')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NCAGES)
			goto bad;
		cg = (int)l;

		/* mod */
		p = s;
		while (isdigit(*s))
			s++;
		if (p == s)
			goto bad;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NMODS)
			goto bad;
		m = (int)l;

		/* temperatures */
		for (i = 0; i < NNODES; i++) {
			while (isspace(*s))
				s++;
			if (*s == '\0')
				break;
			p = s;
			while (isdigit(*s))
				s++;
			if (p == s)
				goto bad;
			if (!isspace(*s))
				goto bad;
			*s++ = '\0';
			if ((l = strtol(p, NULL, 10)) < 0 || l >= INT_MAX)
				goto bad;
			t = (int)l;

			node = &nodes[r][cb][cg][m][i];
			temp = getobj(&t, &temp_list);
			temp->t_cel = t;
			free(temp->t_name);
			temp->t_name = NULL;
			if (asprintf(&temp->t_name, "%dC", t) == -1)
				err(1, "asprintf");
			node->n_fillp = &temp->t_fill;
			node->n_temp = temp;
		}
		continue;
bad:
		warnx("%s:%d: malformed line; %s", _PATH_TEMPMAP, lineno, p);
	}
	if (ferror(fp))
		warn("%s", _PATH_TEMPMAP);
	fclose(fp);
	*fd = -1;
	errno = 0;

	qsort(temp_list.ol_temps, temp_list.ol_tcur, sizeof(struct temp *),
	    temp_cmp);
	for (j = 0; j < temp_list.ol_tcur; j++)
		getcol_temp(j, &temp_list.ol_temps[j]->t_fill);
}

void
parse_mem(int *fd)
{
	FILE *fp;

	if ((fp = fdopen(*fd, "r")) == NULL) {
		warn("fdopen");
		return;
	}
	fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
	    "%*u %*u %*d %*d %*d %*d %*d %*d %*u %lu %ld", &vmem, &rmem);
	fclose(fp);
	*fd = -1;
}

/*
 * Job Id: 4864.phantom.psc.edu
 *   Job_Name = run.cpmd
 *   Job_Owner = stbrown@phantom.psc.edu
 *   resources_used.cput = 00:00:00
 *   resources_used.mem = 11156kb
 *   resources_used.vmem = 29448kb
 *   resources_used.walltime = 02:38:03
 *   job_state = R
 */
void
parse_qstat(int *fd)
{
	char state, *t, *s, *q, buf[BUFSIZ], *next;
	struct job j_fake, *job;
	int jobid;
	FILE *fp;

	s = NULL; /* gcc */
	if ((fp = fdopen(*fd, "r")) == NULL) {
		warn("fdopen");
		return;
	}
	jobid = 0;
	state = '\0';
	for (;;) {
		next = fgets(buf, sizeof(buf), fp);
		if (next == NULL && ferror(fp)) {
			warn("%s", _PATH_QSTAT);
			break;
		}

		q = "Job Id: ";
		if (next == NULL || (s = strstr(buf, q)) != NULL) {
			/* Save last job info. */
			if (state == 'R' &&
			    (job = job_findbyid(jobid)) != NULL) {
				job->j_ncpus = j_fake.j_ncpus;
				job->j_tmdur = j_fake.j_tmdur;
				job->j_tmuse = j_fake.j_tmuse;
				memcpy(job->j_jname, j_fake.j_jname,
				    sizeof(job->j_jname));
				memcpy(job->j_owner, j_fake.j_owner,
				    sizeof(job->j_owner));
				memcpy(job->j_queue, j_fake.j_queue,
				    sizeof(job->j_queue));
			}

			/* Setup next job. */
			jobid = 0;
			state = 0;
			j_fake.j_jname[0] = '\0';
			j_fake.j_owner[0] = '\0';
			j_fake.j_queue[0] = '\0';
			j_fake.j_tmdur = 0;
			j_fake.j_tmuse = 0;
			j_fake.j_ncpus = 0;

			if (feof(fp))
				break;
			else {
				s += strlen(q);
				jobid = strtoul(s, NULL, 10);
			}
		}
		/* buf should always be valid here. */
		if ((t = strchr(buf, '\n')) != NULL)
			*t = '\0';

		q = "Job_Name = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			strncpy(j_fake.j_jname, s,
			    sizeof(j_fake.j_jname) - 1);
			j_fake.j_jname[sizeof(j_fake.j_jname) - 1] = '\0';
		}

		q = "Job_Owner = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			strncpy(j_fake.j_owner, s,
			    sizeof(j_fake.j_owner) - 1);
			j_fake.j_owner[sizeof(j_fake.j_owner) - 1] = '\0';
		}

		q = "job_state = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			state = *s;
		}

		q = "queue = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			strncpy(j_fake.j_queue, s,
			    sizeof(j_fake.j_queue) - 1);
			j_fake.j_queue[sizeof(j_fake.j_queue) - 1] = '\0';
		}

		q = "Resource_List.size = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			j_fake.j_ncpus = strtoul(s, NULL, 10);
		}

		q = "Resource_List.walltime = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			if ((t = strchr(s, ':')) != NULL)
				*t++ = '\0';
			j_fake.j_tmdur = 60 * strtoul(s, NULL, 10);
			j_fake.j_tmdur += strtoul(t, NULL, 10);
		}

		q = "resources_used.walltime = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			if ((t = strchr(s, ':')) != NULL)
				*t++ = '\0';
			j_fake.j_tmuse = 60 * strtoul(s, NULL, 10);
			j_fake.j_tmuse += strtoul(t, NULL, 10);
		}
	}
	fclose(fp);
	*fd = -1;
}
