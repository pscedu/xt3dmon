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

extern int gl_drawhint;
