/* $Id$ */

/*
 * Compile using:
 *	gcc -lGL -lglut mon.c
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
#define _PATH_JOBMAP	"/usr/users/torque/nids_list_login%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"
#define _PATH_PHYSMAP	"/opt/tmp-harness/default/ssconfig/sys%d/nodelist"
*/

#define _PATH_JOBMAP	"/home/yanovich/3d-data/nids_list_login%d"
#define _PATH_PHYSMAP	"/home/yanovich/3d-data/nodelist%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"

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

void			 adjcam(void);
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
GLfloat 		 anglex = -5.0f, angley = 0.1f;
float			 x = -15.0f, y = 4.75f, z = 15.0f;
float			 lx = 0.9f, ly = 0.0f, lz = -0.3f;
int			 spkey, xpos, ypos;
GLint			 cluster_dl;
struct node		*invmap[NROWS * NCABS * NCAGES * NMODS * NNODES];
struct timeval		 lastsync;

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

__inline void
adjcam(void)
{
printf("(%f, %f, %f) -> (%f, %f, %f)\n", x, y, z, lx, ly, lz);
	glLoadIdentity();
	gluLookAt(x, y, z,
	    x + lx, y + ly, z + lz,
	    0.0f, 1.0f, 0.0f);
}

void
reshape(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, w, h);
	gluPerspective(45, 1.0f * w / h, 1, 1000);
	glMatrixMode(GL_MODELVIEW);
	adjcam();
//	glOrtho(-50.0, 50.0, -50.0, 50.0, -150.0, 150.0);
}

void
key(unsigned char key, int x, int y)
{
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
sp_key(int key, int u, int v)
{
	switch (key) {
	case GLUT_KEY_LEFT:
printf("lz: %f [%f]\n", lz, lz * 0.3);
		x += lz * 0.3;
		z -= lx * 0.3;
		break;
	case GLUT_KEY_RIGHT:
		x -= lz * 0.3;
		z += lx * 0.3;
		break;
	case GLUT_KEY_UP:
		x += lx * 0.3;
		z += lz * 0.3;
		break;
	case GLUT_KEY_DOWN:
		x -= lx * 0.3;
		z -= lz * 0.3;
		break;
	case GLUT_KEY_PAGE_UP:
		y += 1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		y -= 1;
		break;
	default:
		return;
	}
	adjcam();
}

void
mouse(int button, int state, int u, int v)
{
	spkey = glutGetModifiers();
}

void
active_m(int u, int v)
{
	int du = u - xpos, dv = v - ypos;

	xpos = u;
	ypos = v;
	if (spkey != GLUT_ACTIVE_CTRL)
		return;
printf("du: %d, dv: %d\n", du, dv);
	if (du != 0) {
		anglex += (du < 0) ? 0.0025f : -0.0025f;
		lx = sin(anglex);
		lz = -cos(anglex);
		adjcam();
	}
	if (dv != 0) {
		angley += (dv < 0) ? 0.0025f : -0.0025f;
		ly = sin(angley);
		lz = -cos(angley);
		adjcam();
	}
}

void
passive_m(int u, int v)
{
}

void
draw(void)
{
	struct timeval tv;
static int cnt = 0;
cnt++;

/*
	if (gettimeofday(&tv, NULL) == -1)
		err(1, "gettimeofday");
	if (lastsync.tv_sec + SLEEP_INTV < tv.tv_sec) {
printf("fps: %d\n", cnt / SLEEP_INTV);
cnt = 0;
		lastsync.tv_sec = tv.tv_sec;
	}
*/


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Ground */
	glColor3f(0.4f, 0.4f, 0.4f);
	glBegin(GL_QUADS);
	glVertex3f( -5.0f, 0.0f, -5.0f);
	glVertex3f( -5.0f, 0.0f, 25.0f);
	glVertex3f(250.0f, 0.0f, 25.0f);
	glVertex3f(250.0f, 0.0f, -5.0f);
	glEnd();

	/* x-axis */
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glVertex3f(-200.0f, -0.1f, -0.1f);
	glVertex3f(-200.0f, -0.1f,  0.1f);
	glVertex3f( 200.0f, -0.1f,  0.1f);
	glVertex3f( 200.0f, -0.1f, -0.1f);

	glVertex3f( 200.0f,  0.1f, -0.1f);
	glVertex3f( 200.0f,  0.1f,  0.1f);
	glVertex3f(-200.0f,  0.1f,  0.1f);
	glVertex3f(-200.0f,  0.1f, -0.1f);
	glEnd();

	/* y-axis */
	glColor3f(0.6f, 0.6f, 1.0f);
	glBegin(GL_QUADS);
	glVertex3f(-0.1f, -200.0f, -0.1f);
	glVertex3f(-0.1f, -200.0f,  0.1f);
	glVertex3f(-0.1f,  200.0f,  0.1f);
	glVertex3f(-0.1f,  200.0f, -0.1f);

	glVertex3f( 0.1f, -200.0f, -0.1f);
	glVertex3f( 0.1f, -200.0f,  0.1f);
	glVertex3f( 0.1f,  200.0f,  0.1f);
	glVertex3f( 0.1f,  200.0f, -0.1f);
	glEnd();

	/* z-axis */
	glColor3f(1.0f, 0.9f, 0.0f);
	glBegin(GL_QUADS);
	glVertex3f(-0.1f, -0.1f, -200.0f);
	glVertex3f(-0.1f,  0.1f, -200.0f);
	glVertex3f(-0.1f,  0.1f,  200.0f);
	glVertex3f(-0.1f, -0.1f,  200.0f);
                                        
	glVertex3f( 0.1f, -0.1f, -200.0f);
	glVertex3f( 0.1f,  0.1f, -200.0f);
	glVertex3f( 0.1f,  0.1f,  200.0f);
	glVertex3f( 0.1f, -0.1f,  200.0f);
	glEnd();

/*
	glLoadIdentity();
	glRotatef(anglex, 1.0, 0.0, 0.0);

	glTranslatef(0.0, -30.0, 0.0);

	glFlush();
*/
	glCallList(cluster_dl);

	glutSwapBuffers();
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
draw_node(struct node *n, float x, float y, float z, float width,
    float height, float depth)
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
	glVertex3f(x, y + height, z);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
	/* Top */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y, z);

	/* Front */
	glVertex3f(x, y + height, z);
	glVertex3f(x, y, z);
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y + height, z);
	/* Back */
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x, y + height, z + depth);

	/* Left */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x, y + height, z);
	/* Right */
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
}

void
draw_mod(float x, float y, float z, float width, float height, float depth)
{
	glPushMatrix();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(1.0f, 0.0f, 0.0f);

	/* Bottom */
	glVertex3f(x, y + height, z);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
	/* Top */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y, z);

	/* Front */
	glVertex3f(x, y + height, z);
	glVertex3f(x, y, z);
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y + height, z);
	/* Back */
	glVertex3f(x, y, z + depth);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x, y + height, z + depth);

	/* Left */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z + depth);
	glVertex3f(x, y + height, z + depth);
	glVertex3f(x, y + height, z);
	/* Right */
	glVertex3f(x + width, y, z);
	glVertex3f(x + width, y, z + depth);
	glVertex3f(x + width, y + height, z + depth);
	glVertex3f(x + width, y + height, z);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();
}

void
make_cluster(void)
{
	int r, cb, cg, m, n;
	float rowdepth, rowspace;
	float cabwidth, cabspace;
	float cageheight, cagespace;
	float modwidth, modheight, moddepth, modspace;
	float nodewidth, nodeheight, nodedepth, nodespace;
	float x = 0.0f, y = 0.0f, z = 0.0f;

	rowspace = 10.0f;
	cabspace = 5.0f;
	cagespace = 1.0f;
	modspace = 1.0f;

	modwidth = 1.0f;
	modheight = 2.0f;
	moddepth = 2.0f;

	nodespace = 0.2f;
	nodewidth = modwidth - 2.0f * nodespace;
	nodeheight = modheight - 4.0f * nodespace;
	nodedepth = modheight - 4.0f * nodespace;
printf("nodewidth: %f, nodedepth: %f\n", nodewidth, nodedepth);

	cageheight = modheight * 2.0f;
	cabwidth = (modwidth + modspace) * NMODS;
	rowdepth = moddepth * 2.0f;

	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
	glBegin(GL_QUADS);
	for (r = 0; r < NROWS; r++, z += rowdepth + rowspace) {
		for (cb = 0; cb < NCABS; cb++, x += cabwidth + cabspace) {
			for (cg = 0; cg < NCAGES; cg++, y += cageheight + cagespace) {
				for (m = 0; m < NMODS; m++, x += modwidth + modspace) {
					for (n = 0; n < NNODES; n++) {
						draw_node(&nodes[r][cb][cg][m][n],
						    x + nodespace,
						    y + (2.0f * nodespace + nodeheight) *
						      (n % (NNODES/2)) + nodespace,
						    z + (2.0f * nodespace + nodedepth) *
						      (n / (NNODES/2)) +
						      (nodespace * (n % (NNODES/2))),
						    nodewidth, nodeheight, nodedepth);
					}
//					draw_mod(x, y, z, modwidth,
//					    modheight, moddepth);
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
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	if (glutCreateWindow("XT3 Monitor") == GL_FALSE)
		errx(1, "CreateWindow");
printf("x: white, y: blue, z: yellow\n");
/*
	glClearColor(0.5, 0.5, 0.5, 0.5);
*/
	glShadeModel(GL_SMOOTH);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);

	parse_physmap();
	parse_jobmap();
	make_cluster();

//	angle = -39.0;

	/* glutExposeFunc(reshape); */
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutSpecialFunc(sp_key);
	glutDisplayFunc(draw);
	glutIdleFunc(draw);
	glutMouseFunc(mouse);
	glutMotionFunc(active_m);
	glutPassiveMotionFunc(passive_m);

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

	if (jobs != NULL)
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
	double div;

	if (njobs == 1)
		div = 0.0;
	else
		div = n / (njobs - 1);

	j->j_r = cos(div);
	j->j_g = sin(div) * sin(div);
	j->j_b = fabs(tan(div + PI * 3/4));
}
