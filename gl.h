/* $Id$ */

/* gl.c */
void		 gl_idleh_default(void);
void		 gl_idleh_govern(void);
void		 gl_reshapeh(int, int);
void		 gl_run(void (*)(void));
void		 gl_setidleh(void);
void		 gl_setup(void);
void		 gl_setup_core(void);
void		 gl_wid_update(void);
void		 gl_displayh_default(void);
void		 gl_displayh_stereo(void);
void		 gl_setbg(void);
struct glname	*gl_select(int);

/* key.c */
void		 gl_keyh_actflyby(unsigned char, int, int);
void		 gl_keyh_default(unsigned char, int, int);
void		 gl_keyh_uinput(unsigned char, int, int);
void		 gl_keyh_server(unsigned char, int, int);

void		 gl_spkeyh_actflyby(int, int, int);
void		 gl_spkeyh_default(int, int, int);
void		 gl_spkeyh_node(int, int, int);
void		 gl_spkeyh_wiadj(int, int, int);

/* mouse.c */
void		 gl_motionh_default(int, int);
void		 gl_motionh_null(int, int);
void		 gl_motionh_panel(int, int);
void		 gl_motionh_select(int, int);
void		 gl_mouseh_default(int, int, int, int);
void		 gl_mouseh_null(int, int, int, int);
void		 gl_pasvmotionh_default(int, int);
void		 gl_pasvmotionh_null(int, int);
void		 gl_mwheel_default(int, int, int, int);
