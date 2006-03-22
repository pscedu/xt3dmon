/* $Id$ */

struct state;

/* Flyby modes. */
#define FBM_OFF		0
#define FBM_PLAY	1
#define FBM_REC		2

void 	 flyby_begin(int);
void 	 flyby_end(void);
void	 flyby_read(void);
void	 flyby_update(void);
void	 flyby_writeinit(struct state *);
void	 flyby_writeseq(struct state *);
void	 flyby_writepanel(int);
void	 flyby_writeselnode(int);
void	 flyby_writehlsc(int);

extern int flyby_mode;