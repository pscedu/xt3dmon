/* $Id$ */

/*
 * Compile using:
 *	gcc -lGL -lglut mon.c
 */

#include <sys/param.h>
#include <sys/queue.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define _PATH_JOBMAP	"/usr/users/torque/nids_list_login%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"
#define _PATH_PHYSMAP	"/opt/tmp-harness/default/ssconfig/sys%d/nodelist"

#define WIN_WIDTH	800
#define WIN_HEIGHT	600

#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

#define PI		3.14159265358979323

#define NID_MAX		(NROWS * NCABS * NCAGES * NMODS * NNODES)

#define SLEEP_INTV	5

#define DIR_X 0
#define DIR_Y 1
#define DIR_Z 2

struct job {
	int		j_id;
	float		j_r;
	float		j_g;
	float		j_b;
};

struct node {
	int		 n_nid;
	int		 n_logid;
	int		 n_jobid;
	int		 n_state;
};

struct state {
	char		*st_name;
	float		 st_r;
	float		 st_g;
	float		 st_b;
};

void			 orient(float);
void			 move(int);
void			 parse_jobmap(void);
void			 parse_physmap(void);
struct job		*getjob(int);
void			 getcol(int, struct job *);

int			 logids[] = { 0, 8 };
#define NLOGIDS		(sizeof(logids) / sizeof(logids[0]))

struct job		**jobs;
size_t			 njobs;
size_t			 maxjobs;
struct node		 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
GLfloat 		 angle;
float			 x, y, z, lx, ly, lz;
GLint			 cluster_dl;
struct node		*invmap[NROWS * NCABS * NCAGES * NMODS * NNODES];

struct state states[] = {
#define ST_FREE		0
	{ "Free",		1.0, 1.0, 1.0 },
#define ST_DOWN		1
	{ "Disabled (PBS)",	1.0, 0.0, 0.0 },
#define ST_DISABLED	2
	{ "Disabled (HW)",	0.66, 0.66, 0.66 },
#define ST_USED		3
	{ NULL,			0.0, 0.0, 0.0 },
#define ST_IO		4
	{ "I/O",		1.0, 1.0, 0.0 },
#define ST_UNACC	5
	{ "Unaccounted",	0.0, 0.0, 1.0 },
#define ST_BAD		6
	{ "Bad",		1.0, 0.75, 0.75 },
#define ST_CHECK	7
	{ "Checking",		0.0, 1.0, 0.0 }
#define NST		8
};

void
reshape(int width, int height)
{
printf("reshape()\n");
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-50.0, 50.0, -50.0, 50.0, -150.0, 150.0);
	glMatrixMode(GL_MODELVIEW);
}

void
key(unsigned char key, int x, int y)
{
printf("key pressed\n");
	switch (key) {
	case 'q':
	case 'Q':
		exit(0);
		/* NOTREACHED */
	}
}

/*
 *	       z
 *	      /
 *	     / PageUp /\
 *	    /
 *	   /
 *	  / PageDown \/
 *	 /
 *	+------------------------------------------ x
 *	|	Left <-		-> Right
 *	|
 *	|
 *	| Up /\
 *	|
 *	|
 *	| Down \/
 *	|
 *	|
 *	y
 */
void
sp_key(int key, int x, int y)
{
printf("special key pressed (%d)\n", key);
	switch (key) {
	case GLUT_KEY_LEFT:
		angle -= 0.8f;
		orient(angle);
		break;
	case GLUT_KEY_RIGHT:
		angle += 0.8f;
		orient(angle);
		break;
	case GLUT_KEY_UP:
		move(1);
		break;
	case GLUT_KEY_DOWN:
		move(-1);
		break;
	case GLUT_KEY_PAGE_UP:
		move(1);
		break;
	case GLUT_KEY_PAGE_DOWN:
		move(-1);
		break;
	}
}

void
orient(float angle)
{
	lx = sin(angle);
	ly = -cos(angle);
	glLoadIdentity();
printf("[%f,%f] [%f,%f] [%f,%f]\n", x, lx, y, ly, z, 0);
	gluLookAt(x, y, z,
	    x + lx, y + ly, z,
	    0.0f, 1.0f, 0.0f);
}

void
move(int dir)
{
	x += dir * lx * 0.1;
	z += dir * lz * 0.1;
printf("[%f,%f] [%f,%f] [%f,%f]\n", x, lx, y, ly, z, lz);
	glLoadIdentity();
	gluLookAt(x, y, z,
	    x + lx, y + ly, z + lz,
	    0.0f, 1.0f, 0.0f);
}

void
draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glRotatef(angle, 1.0, 0.0, 0.0);
	glTranslatef(0.0, -30.0, 0.0);
	glCallList(cluster_dl);

	glFlush();
	glutSwapBuffers();
usleep(750);
}

/*
 *             z
 *            /
 *           /
 *          /
 * (0,0,0) +-------------------------------------------------------- x
 *         |                 (x,y,z+DEPTH)   (x+WIDTH,y,z+DEPTH)
 *         |                         +----------+
 *         |                        /   Top    /|
 *         |                       / |        / |
 *         |              (x,y,z) +----------+  | (x+WIDTH,y,z)
 *         |                      |   (Back) |  |
 *         |                      |  |       |  | Right
 *         |                      |          |  |
 *         |                 Left |  | Front |  |
 *         |                      |          |  |
 *         | (x,y+HEIGHT,z+DEPTH) |  +  -  - |- + (x+WIDTH,y+HEIGHT,z+DEPTH)
 *         |                      | /        | /
 *         |                      | (Bottom) |/
 *         |                      +----------+
 *         |        (x,y+HEIGHT,z) (x+WIDTH,y+HEIGHT,z)
 *         |
 *         y
 */
__inline void
draw_node(struct node *n, int x, int y, int z, int width, int height, int depth)
{
	float r, g, b;

	if (n->n_state == ST_USED)
		r = states[n->n_state].st_r;
	else {
		r = states[n->n_state].st_r;
		g = states[n->n_state].st_g;
		b = states[n->n_state].st_b;
	}
	glColor3f(r, g, b);

	/* Bottom */
	glVertex3d(x, y + height, z);
	glVertex3d(x, y + height, z + depth);
	glVertex3d(x + width, y + height, z + depth);
	glVertex3d(x + width, y + height, z);
	/* Top */
	glVertex3d(x, y, z);
	glVertex3d(x, y, z + depth);
	glVertex3d(x + width, y, z + depth);
	glVertex3d(x + width, y, z);

	/* Front */
	glVertex3d(x, y + height, z);
	glVertex3d(x, y, z);
	glVertex3d(x + width, y, z);
	glVertex3d(x + width, y + height, z);
	/* Back */
	glVertex3d(x, y, z + depth);
	glVertex3d(x + width, y, z + depth);
	glVertex3d(x + width, y + height, z + depth);
	glVertex3d(x, y + height, z + depth);

	/* Left */
	glVertex3d(x, y, z);
	glVertex3d(x, y, z + depth);
	glVertex3d(x, y + height, z + depth);
	glVertex3d(x, y + height, z);
	/* Right */
	glVertex3d(x + width, y, z);
	glVertex3d(x + width, y, z + depth);
	glVertex3d(x + width, y + height, z + depth);
	glVertex3d(x + width, y + height, z);
}

void
make_cluster(void)
{
	int r, cb, cg, m, n;
	int rowdepth, rowspace;
	int cabwidth, cabspace;
	int cageheight, cagespace;
	int modwidth, modspace;
	int nodewidth, nodeheight, nodedepth;

	rowspace = 50;
	cabspace = 25;
	cagespace = 10;
	modspace = 5;

	nodewidth = nodeheight = nodedepth = 10;
	modwidth = nodewidth;
	cageheight = nodeheight * 2;
	cabwidth = modwidth * NMODS;
	rowdepth = nodedepth * 2;
printf("make_cluster()\n");
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
	glBegin(GL_QUADS);
	for (r = 0; r < NROWS; r++, z += rowdepth + rowspace) {
		for (cb = 0; cb < NCABS; cb++, x += cabwidth + cabspace) {
			for (cg = 0; cg < NCAGES; cg++, y += cageheight + cagespace) {
				for (m = 0; m < NMODS; m++, x += modwidth + modspace) {
					for (n = 0; n < NNODES; n++) {
						draw_node(&nodes[r][cb][cg][m][n], x,
						    y + nodeheight * (n % (NNODES/2)),
						    z + nodedepth * (n / (NNODES/2)),
						    nodewidth, nodeheight, nodedepth);
goto end;
					}
				}
				x -= (modwidth + modspace) * NMODS;
			}
			y -= (cageheight + cagespace) * NCAGES;
		}
		x -= (cabwidth + cabspace) * NCABS;
	}
end:
	glEnd();
	glEndList();
}

int
main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	if (glutCreateWindow("Monitor") == GL_FALSE)
		errx(1, "CreateWindow");

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);

	parse_physmap();
	make_cluster();

	angle = -39.0;

	/* glutExposeFunc(reshape); */
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutSpecialFunc(sp_key);
	glutDisplayFunc(draw);
	glutIdleFunc(draw);
	glutMainLoop();
	exit(0);
}

/*
 * Example line:
 *	c0-1c0s1 3 7 c
 *
 * Broken up:
 *	cb r cg  m	n nid  state
 *	c0-1 c0 s1	3 7    c
 */
void
parse_physmap(void)
{
	char fn[MAXPATHLEN], buf[BUFSIZ], *p, *s;
	int j, lineno, r, cb, cg, m, n, nid;
	struct node *node;
	FILE *fp;
	long l;

	/* Explicitly initialize all nodes. */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++)
						nodes[r][cb][cg][m][n].n_state = ST_UNACC;

	for (j = 0; j < NLOGIDS; j++) {
		snprintf(fn, sizeof(fn), _PATH_PHYSMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL) {
			warn("%s", fn);
			continue;
		}
		lineno = 0;
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			lineno++;
			p = buf;
			while (isspace(*p))
				p++;
			if (*p == '#')
				continue;
			if (*p++ != 'c')
				goto bad;
			if (!isdigit(*p))
				goto bad;

			/* cb */
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
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NCAGES)
				goto bad;
			cg = (int)l;

			/* m */
			p = s + 1;
			s = p;
			while (isdigit(*++s))
				;
			if (!isspace(*s))
				goto bad;
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NMODS)
				goto bad;
			m = (int)l;

			/* n */
			while (isspace(*++s))
				;
			if (!isdigit(*s))
				goto bad;
			p = s;
			while (isdigit(*++s))
				;
			if (!isspace(*s))
				goto bad;
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NNODES)
				goto bad;
			n = (int)l;

			/* nid */
			while (isspace(*++s))
				;
			if (!isdigit(*s))
				goto bad;
			p = s;
			while (isdigit(*++s))
				;
			if (!isspace(*s))
				goto bad;
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NID_MAX)
				goto bad;
			nid = (int)l;

			node = &nodes[r][cb][cg][m][n];
			node->n_nid = nid;
			node->n_logid = logids[j];
			invmap[nid] = node;

			/* state */
			while (isspace(*++s))
				;
			if (!isalpha(*s))
				goto bad;
			switch (*s) {
			case 'c': /* compute */
				node->n_state = ST_FREE;	/* Good enough. */
				break;
			case 'n': /* disabled */
				node->n_state = ST_DISABLED;
				break;
			case 'i': /* I/O */
				node->n_state = ST_IO;
				break;
			default:
				goto bad;
			}
			continue;
bad:
			warnx("%s:%d: malformed line [%s] [%s]", fn, lineno, buf, p);
		}
		if (ferror(fp))
			warn("%s", fn);
		fclose(fp);
		errno = 0;
	}
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
parse_jobmap(void)
{
	int jobid, nid, j, lineno, enabled, bad, checking;
	char fn[MAXPATHLEN], buf[BUFSIZ], *p, *s;
	struct node *node;
	FILE *fp;
	long l;

	for (j = 0; j < njobs; j++)
		free(jobs[j]);
	njobs = 0;
	for (j = 0; j < NLOGIDS; j++) {
		snprintf(fn, sizeof(fn), _PATH_JOBMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL) {
			warn("%s", fn);
			continue;
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

			node = invmap[nid];
			if (node == NULL && enabled) {
				warnx("inconsistency: node %d should be "
				    "disabled in jobmap", nid);
				continue;
			}

			if (enabled == 0)
				node->n_state = ST_DOWN;
			else if (jobid == 0)
				node->n_state = ST_FREE;
			else {
				node->n_state = ST_USED;
				node->n_jobid = jobid;
				getjob(jobid);
			}
			continue;
bad:
			warn("%s:%d: malformed line", fn, lineno);
		}
		if (ferror(fp))
			warn("%s", fn);
		fclose(fp);
		errno = 0;

		snprintf(fn, sizeof(fn), _PATH_BADMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL)
			/* This is OK. */
			errno = 0;
		else {
			lineno = 0;
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				lineno++;
				p = buf;
				while (isspace(*p))
					p++;
				if (*p == '#')
					continue;
				if (!isdigit(*p))
					goto badbad;

				/* nid */
				s = p;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badbad;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
					goto badbad;
				nid = (int)l;

				/* disabled */
				p = s + 1;
				s = p;
				if (!isdigit(*s))
					goto badbad;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badbad;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
					goto badbad;
				bad = (int)l;

				if (bad)
					/* XXX:  check validity. */
					invmap[nid]->n_state = ST_BAD;
				continue;
badbad:
				warnx("%s:%d: malformed line", fn, lineno);
				}
			if (ferror(fp))
				warn("%s", fn);
			fclose(fp);
			errno = 0;
		}

		snprintf(fn, sizeof(fn), _PATH_CHECKMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL)
			/* This is OK. */
			errno = 0;
		else {
			lineno = 0;
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				lineno++;
				p = buf;
				while (isspace(*p))
					p++;
				if (*p == '#')
					continue;
				if (!isdigit(*p))
					goto badcheck;

				/* nid */
				s = p;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badcheck;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
					goto badcheck;
				nid = (int)l;

				/* disabled */
				p = s + 1;
				s = p;
				if (!isdigit(*s))
					goto badcheck;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badcheck;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
					goto badcheck;
				checking = (int)l;

				if (checking)
					/* XXX:  check validity. */
					invmap[nid]->n_state = ST_CHECK;
				continue;
badcheck:
				warnx("%s:%d: malformed line", fn, lineno);
			}
			if (ferror(fp))
				warn("%s", fn);
			fclose(fp);
			fclose(fp);
			errno = 0;
		}
	}

	for (j = 0; j < njobs; j++)
		getcol(j, jobs[j]);
}

#define JINCR 10

struct job *
getjob(int id)
{
	struct job **jj, *j = NULL;
	int n;

	if (jobs == NULL)
		for (n = 0, jj = jobs; n < njobs; jj++, n++)
			if ((*jj)->j_id == id)
				return (j);
	if (njobs + 1 >= maxjobs) {
		maxjobs += JINCR;
		if ((jobs = realloc(jobs, sizeof(*jobs) * maxjobs)) == NULL)
			err(1, "realloc");
	}
	if ((j = malloc(sizeof(*j))) == NULL)
		err(1, "malloc");
	j->j_id = id;
	jobs[njobs++] = j;
	return (j);
}

void
getcol(int n, struct job *j)
{
	double div = n / (njobs - 1);

	j->j_r = cos(div);
	j->j_g = sin(div) * sin(div);
	j->j_b = fabs(tan(div + PI * 3/4));
}

