/* $Id$ */

/*
#define _PATH_JOBMAP	"/usr/users/torque/nids_list_login%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"
#define _PATH_PHYSMAP	"/opt/tmp-harness/default/ssconfig/sys%d/nodelist"
*/

#define _PATH_JOBMAP	"data/nids_list_login%d"
#define _PATH_PHYSMAP	"data/nodelist%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"

#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

#define ST_FREE		0
#define ST_DOWN		1
#define ST_DISABLED	2
#define ST_USED		3
#define ST_IO		4
#define ST_UNACC	5
#define ST_BAD		6
#define ST_CHECK	7
#define NST		8

#define PI		3.14159265358979323

#define NLOGIDS		(sizeof(logids) / sizeof(logids[0]))

//#define TEX_SIZE 128

struct job {
	int		 j_id;
	float		 j_r;
	float		 j_g;
	float		 j_b;
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
	int		 st_texid;
};

void			 parse_jobmap(void);
void			 parse_physmap(void);
void*			 LoadPNG(char *file);
void			 LoadTexture(void *data, int id);

extern struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	*invmap[NROWS * NCABS * NCAGES * NMODS * NNODES];
extern size_t		 njobs;
extern struct job	**jobs;
