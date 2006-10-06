/* $Id$ */

struct fvec;
struct state;

/* Flyby modes. */
#define FBM_OFF			0
#define FBM_PLAY		1
#define FBM_REC			2

/* Flyby node class highlighting operations -- must stay sync'd to hl.c. */
#define FBHLOP_UNKNOWN		(-1)
#define FBHLOP_XPARENT		0
#define FBHLOP_OPAQUE		1
#define FBHLOP_ALPHAINC		2
#define FBHLOP_ALPHADEC		3
#define FBHLOP_TOGGLEVIS	4

#define FLYBY_DEFAULT		"default"

char	*flyby_set(const char *, int);
void 	 flyby_begin(int);
void 	 flyby_beginauto(void);
void 	 flyby_end(void);
void	 flyby_read(void);
void	 flyby_update(void);
void	 flyby_writeinit(struct state *);
void	 flyby_writeseq(struct state *);
void	 flyby_writepanel(int);
void	 flyby_writeselnode(int, const struct fvec *);
void	 flyby_writehlnc(int, int);
void	 flyby_writecaption(const char *);
void	 flyby_rstautoto(void);
void	 flyby_clear(void);

extern int	flyby_mode;
extern int 	flyby_nautoto;
extern char 	flyby_fn[];
extern char 	flyby_dir[];
