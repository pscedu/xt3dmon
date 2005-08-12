/* $Id$ */

/* Easter Eggs, Inside Jokes, and the Like... */

#include"compat.h"
#include"mon.h"

// XXX #define BORG_FONT "data/borg_font.png"
#define BORG_PATH "data/borg.png"
#define BORG_ACT 	(1<<0)
#define BORG_ON 	(1<<1)

/* Borge Texture Load */
void load_borg_textures(void)
{
	void *data;
	unsigned int i;

	/* Read in texture IDs. */
	for (i = 0; i < NJST; i++) {
		data = load_png(BORG_PATH);
		load_texture(data, jstates[i].js_fill.f_alpha_fmt,
				GL_RGBA, i + 1);
		jstates[i].js_fill.f_texid = i + 1; 
	}

	/* Load the font texture */
	font_id = i + 1;
	data = load_png(_PATH_FONT);

	/* This puts background color over white in texture */
	load_texture(data, GL_INTENSITY, GL_RGBA, font_id);
}

/*
** Borg Mode Easter Egg 
** returns 1 if borg mode is on and it's textures
** have been properly loaded.
*/
int borg_mode(int toggle)
{
	static int on = 0;
	static struct state ost;

	/* Just toggle borg mode, don't activate yet */
	if (toggle) {
		on ^= BORG_ON;
		return ((on & BORG_ON) != 0);
	}

	if ((on & BORG_ON) != 0) {

		del_textures();
		load_borg_textures();

		/* If borg is active, don't resave state */
		if((on & BORG_ACT) != 0) {
			return ((on & BORG_ON) != 0);
		}

		on |= BORG_ACT;

		/* Save current state */
		ost = st;

		/* Set up Borg view =) */
		st.st_vmode = VM_WIREDONE;
		st.st_opts |= OP_TEX;
		st.st_opts &= (~OP_GROUND);
		st.st_rf |= RF_CLUSTER;

		tween_push(TWF_LOOK | TWF_POS);
		st.st_x = -89.0; 	st.st_lx = 1.0;
		st.st_y = 25.0;		st.st_ly = 0.0;
		st.st_z = 35.0;		st.st_lz = 0.0;
		tween_pop(TWF_LOOK | TWF_POS);

		refresh_state(ost.st_opts);

	} else if ((on & BORG_ACT) != 0) {
		int opts;

		/* Borg now inactive */
		on ^= BORG_ACT;

		del_textures();

		/* Restore state from before borg mode */
		opts = st.st_opts;
		st.st_opts = ost.st_opts;
		st.st_vmode = ost.st_vmode;

		tween_push(TWF_LOOK | TWF_POS);
		st.st_x = ost.st_x;	st.st_lx = ost.st_lx;
		st.st_y = ost.st_y; 	st.st_ly = ost.st_ly;
		st.st_z = ost.st_z; 	st.st_lz = ost.st_lz;
		tween_pop(TWF_LOOK | TWF_POS);
		
		st.st_rf |= RF_CLUSTER;
	}

	return ((on & BORG_ON) != 0);
}

/* 
** Toggle and Update Easter Eggs
** return nonzero if this function will override
** the calling functions functionality...
*/
int easter_eggs(int opt)
{
	static int eggs = 0;
	int ret = 0;
	int diff;

	/* Find changes */
	diff = eggs ^ (eggs ^ opt);

	/* Update or toggle occured */
	if(diff & EGG_BORG || diff & EGG_UPDATE)
		ret += borg_mode((diff & EGG_BORG) != 0);

	/* Apply changes */
	eggs ^= opt;
	
	return ret;
}

