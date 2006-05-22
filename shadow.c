/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "draw.h"
#include "env.h"
#include "fill.h"
#include "gl.h"
#include "node.h"
#include "select.h"
#include "state.h"
#include "xmath.h"

/*
 * The "shadow" routines in this file draw simple objects
 * in multiple steps to try and find a node that was been
 * selected.
 */

/*
 * A wired selection step models some subdivision of the cube.
 * It contains which 'chance' (i.e. try) we are analyzing hits
 * at that level and the dimensions of the sub cube.
 */
struct wiselstep {
	int		ws_chance;
	struct ivec	ws_off;
	struct ivec	ws_mag;
};

#if 0
void
draw_shadow_physcubes(int n, struct fvec *fvp, struct fvec *dimp,
    struct fvec *inc)
{
	struct physdim *pd;

	for (pd = physdim_top; pd != NULL; pd = pd->contains) {
		glPushMatrix();
		glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z);
		glPushName(gsn_get(i, NULL, 0));
		draw_cube(dimp, &fill_black, 0);
		glPopName();
		glPopMatrix();
	}
}
#endif

void
draw_shadow_rows(void)
{
	struct fvec dim, v;
	int r;

	v.fv_x = NODESPACE;
	v.fv_y = NODESPACE;
	v.fv_z = NODESPACE;

	dim.fv_w = ROWWIDTH;
	dim.fv_h = CABHEIGHT;
	dim.fv_d = ROWDEPTH + NODESHIFT;

	for (r = 0; r < NROWS; r++,
	    v.fv_z += ROWSPACE + NODESHIFT + ROWDEPTH) {
		glPushMatrix();
		glTranslatef(v.fv_x, v.fv_y, v.fv_z);
		glPushName(gsn_get(r, NULL, 0));
		draw_cube(&dim, &fill_black, 0);
		glPopName();
		glPopMatrix();
	}
}

void
draw_shadow_cabs(const struct physcoord *pc)
{
	struct fvec dim, v;
	int cb;

	v.fv_x = NODESPACE;
	v.fv_y = NODESPACE;
	v.fv_z = NODESPACE + pc->pc_r * (ROWSPACE + NODESHIFT + ROWDEPTH);

	dim.fv_w = CABWIDTH + NODEWIDTH;
	dim.fv_h = CABHEIGHT;
	dim.fv_d = ROWDEPTH + NODESHIFT;

	for (cb = 0; cb < NCABS; cb++, v.fv_x += CABWIDTH + CABSPACE) {
		glPushMatrix();
		glTranslatef(v.fv_x, v.fv_y, v.fv_z);
		glPushName(gsn_get(cb, NULL, 0));
		draw_cube(&dim, &fill_black, 0);
		glPopName();
		glPopMatrix();
	}
}

void
draw_shadow_cages(const struct physcoord *pc)
{
	struct fvec dim, v;
	int cg;

	v.fv_x = NODESPACE + pc->pc_cb * (CABSPACE + CABWIDTH);
	v.fv_y = NODESPACE;
	v.fv_z = NODESPACE + pc->pc_r * (ROWSPACE + NODESHIFT + ROWDEPTH);

	dim.fv_w = CABWIDTH + NODEWIDTH;
	dim.fv_h = CAGEHEIGHT;
	dim.fv_d = ROWDEPTH + NODESHIFT;

	for (cg = 0; cg < NCAGES; cg++, v.fv_y += CAGEHEIGHT + CAGESPACE) {
		glPushMatrix();
		glTranslatef(v.fv_x, v.fv_y, v.fv_z);
		glPushName(gsn_get(cg, NULL, 0));
		draw_cube(&dim, &fill_black, 0);
		glPopName();
		glPopMatrix();
	}
}

__inline void
draw_shadow_node(int *dl, struct node *n)
{
	if (dl[n->n_geom]) {
		glCallList(dl[n->n_geom]);
		return;
	}

	dl[n->n_geom] = glGenLists(1);
	glNewList(dl[n->n_geom], GL_COMPILE_AND_EXECUTE);
	switch (n->n_geom) {
	case GEOM_SPHERE:
		draw_sphere(n->n_dimp, &fill_black, 0);
		break;
	case GEOM_CUBE:
		draw_cube(n->n_dimp, &fill_black, 0);
		break;
	}
	glEndList();
}

void
draw_shadow_mods(const struct physcoord *pc, int *dl)
{
	struct node *node;
	int m, n;

	for (m = 0; m < NMODS; m++) {
		for (n = 0; n < NNODES; n++) {
			node = &nodes[pc->pc_r][pc->pc_cb][pc->pc_cg][m][n];
			if (!node_show(node))
				continue;

			glPushMatrix();
			glTranslatef(node->n_v->fv_x,
			    node->n_v->fv_y, node->n_v->fv_z);
			glPushName(gsn_get(node->n_nid, gscb_node, 0));
			draw_shadow_node(dl, node);
			glPopName();
			glPopMatrix();
		}
	}
}

void
draw_shadow_winodes(int *dl, struct wiselstep *ws, const struct fvec *cloffp)
{
	struct ivec iv, *mag, *off, adjv;
	struct node *node;

	mag = &ws->ws_mag;
	off = &ws->ws_off;

	for (iv.iv_x = off->iv_x; iv.iv_x < off->iv_x + mag->iv_x; iv.iv_x++)
		for (iv.iv_y = off->iv_y; iv.iv_y < off->iv_y + mag->iv_y; iv.iv_y++)
			for (iv.iv_z = off->iv_z; iv.iv_z < off->iv_z + mag->iv_z; iv.iv_z++) {
				adjv.iv_x = negmod(iv.iv_x + st.st_wioff.iv_x, WIDIM_WIDTH);
				adjv.iv_y = negmod(iv.iv_y + st.st_wioff.iv_y, WIDIM_HEIGHT);
				adjv.iv_z = negmod(iv.iv_z + st.st_wioff.iv_z, WIDIM_DEPTH);
				node = wimap[adjv.iv_x][adjv.iv_y][adjv.iv_z];
				if (node == NULL || !node_show(node))
					continue;

				glPushMatrix();
				glTranslatef(
				    node->n_v->fv_x + cloffp->fv_x,
				    node->n_v->fv_y + cloffp->fv_y,
				    node->n_v->fv_z + cloffp->fv_z);
				glPushName(gsn_get(node->n_nid, gscb_node, 0));
				draw_shadow_node(dl, node);
				glPopName();
				glPopMatrix();
			}
}

void
draw_shadow_wisect(int *dl, struct wiselstep *ws, int cuts, int last,
    const struct fvec *cloffp)
{
	struct fvec onv, nv, dim, adj;
	struct ivec mag, len, *offp;
	int cubeno;

	if (last) {
		draw_shadow_winodes(dl, ws, cloffp);
		return;
	}

	offp = &ws->ws_off;

	len = mag = ws->ws_mag;
	len.iv_x = round(mag.iv_x / (double)cuts);
	len.iv_y = round(mag.iv_y / (double)cuts);
	len.iv_z = round(mag.iv_z / (double)cuts);

	nv.fv_x = offp->iv_x * st.st_winsp.iv_x;
	nv.fv_y = offp->iv_y * st.st_winsp.iv_y;
	nv.fv_z = offp->iv_z * st.st_winsp.iv_z;

	if (st.st_winsp.iv_x < 0)
		nv.fv_x += NODEWIDTH;
	if (st.st_winsp.iv_y < 0)
		nv.fv_y += NODEHEIGHT;
	if (st.st_winsp.iv_z < 0)
		nv.fv_z += NODEDEPTH;

	onv = nv;

	adj.fv_x = len.iv_x * st.st_winsp.iv_x;
	adj.fv_y = len.iv_y * st.st_winsp.iv_y;
	adj.fv_z = len.iv_z * st.st_winsp.iv_z;

	cubeno = 0;
	for (; mag.iv_x > 0; nv.fv_x += adj.fv_x) {
		mag.iv_x -= len.iv_x;
		if (mag.iv_x < len.iv_x / 2) {
			len.iv_w += mag.iv_x;
			mag.iv_x = 0;
		}
		dim.fv_w = (len.iv_x - 1) * st.st_winsp.iv_x + NODEWIDTH *
		    SIGNF(st.st_winsp.iv_x);

		/*
		 * Restore magnitude/length as it may have been
		 * modified by the last loop iteration below.
		 *
		 * The same applies below to 'z'.
		 */
		mag.iv_y = ws->ws_mag.iv_y;
		len.iv_y = mag.iv_y / cuts;
		for (; mag.iv_y > 0; nv.fv_y += adj.fv_y) {
			mag.iv_y -= len.iv_y;
			if (mag.iv_y < len.iv_y / 2) {
				len.iv_h += mag.iv_y;
				mag.iv_y = 0;
			}
			dim.fv_h = (len.iv_y - 1) * st.st_winsp.iv_y + NODEHEIGHT *
			    SIGNF(st.st_winsp.iv_y);

			mag.iv_z = ws->ws_mag.iv_z;
			len.iv_z = mag.iv_z / cuts;
			for (; mag.iv_z > 0; nv.fv_z += adj.fv_z) {
				mag.iv_z -= len.iv_z;
				if (mag.iv_z < len.iv_z / 2) {
					len.iv_d += mag.iv_z;
					mag.iv_z = 0;
				}
				dim.fv_d = (len.iv_z - 1) * st.st_winsp.iv_z + NODEDEPTH *
				    SIGNF(st.st_winsp.iv_z);

				glPushMatrix();
				glTranslatef(
				    nv.fv_x + st.st_wioff.iv_x * st.st_winsp.iv_x + cloffp->fv_x,
				    nv.fv_y + st.st_wioff.iv_y * st.st_winsp.iv_y + cloffp->fv_y,
				    nv.fv_z + st.st_wioff.iv_z * st.st_winsp.iv_z + cloffp->fv_z);
				glPushName(gsn_get(cubeno++, NULL, 0));
				draw_cube(&dim, &fill_black, 0);
				glPopName();
				glPopMatrix();
			}
			nv.fv_z = onv.fv_z;
		}
		nv.fv_y = onv.fv_y;
	}
}

static void
cubeno_to_v(int cubeno, int cuts, struct wiselstep *ws)
{
	struct ivec iv, len, *magp;
	struct wiselstep *pws;
	int n;

	pws = ws - 1;
	magp = &pws->ws_mag;
	ws->ws_off = pws->ws_off;

	len = *magp;
	len.iv_x = round(magp->iv_x / (double)cuts);
	len.iv_y = round(magp->iv_y / (double)cuts);
	len.iv_z = round(magp->iv_z / (double)cuts);

	iv.iv_x = cubeno / cuts / cuts;
	cubeno %= cuts * cuts;

	iv.iv_y = cubeno / cuts;
	cubeno %= cuts;

	iv.iv_z = cubeno;

	ws->ws_off.iv_x += iv.iv_x * len.iv_x;
	ws->ws_off.iv_y += iv.iv_y * len.iv_y;
	ws->ws_off.iv_z += iv.iv_z * len.iv_z;

	ws->ws_mag.iv_x = len.iv_x;
	ws->ws_mag.iv_y = len.iv_y;
	ws->ws_mag.iv_z = len.iv_z;

	/* Shorten/extend the magnitude as needed. */
	n = ws->ws_off.iv_x + ws->ws_mag.iv_x;
	if (n > magp->iv_x)
		ws->ws_mag.iv_x -= n - magp->iv_x;
	else if (magp->iv_x - n < len.iv_x / 2)
		ws->ws_mag.iv_x += magp->iv_x - n;

	n = ws->ws_off.iv_y + ws->ws_mag.iv_y;
	if (n > magp->iv_y)
		ws->ws_mag.iv_y -= n - magp->iv_y;
	else if (magp->iv_y - n < len.iv_y / 2)
		ws->ws_mag.iv_y += magp->iv_y - n;

	n = ws->ws_off.iv_z + ws->ws_mag.iv_z;
	if (n > magp->iv_z)
		ws->ws_mag.iv_z -= n - magp->iv_z;
	else if (magp->iv_z - n < len.iv_z / 2)
		ws->ws_mag.iv_z += magp->iv_z - n;
}

__inline int
phys_select(int *dl, int flags)
{
	struct physcoord pc, chance;
	int nrecs, id;

	for (chance.pc_r = 0; chance.pc_r < NROWS; chance.pc_r++) {
		sel_begin();
		draw_shadow_rows();
		nrecs = sel_end();
		if (nrecs == 0 || (pc.pc_r =
		    sel_process(nrecs, chance.pc_r, flags)) == SP_MISS)
			break;
		for (chance.pc_cb = 0; chance.pc_cb < NCABS; chance.pc_cb++) {
			sel_begin();
			draw_shadow_cabs(&pc);
			nrecs = sel_end();
			if (nrecs == 0 || (pc.pc_cb =
			    sel_process(nrecs, chance.pc_cb, flags)) == SP_MISS)
				break;
			for (chance.pc_cg = 0; chance.pc_cg < NCAGES; chance.pc_cg++) {
				sel_begin();
				draw_shadow_cages(&pc);
				nrecs = sel_end();
				if (nrecs == 0 || (pc.pc_cg =
				    sel_process(nrecs, chance.pc_cg, flags)) == SP_MISS)
					break;
				for (chance.pc_m = 0; chance.pc_m < NMODS; chance.pc_m++) {
					sel_begin();
					draw_shadow_mods(&pc, dl);
					nrecs = sel_end();
					if (nrecs && (id =
					    sel_process(nrecs, 0, flags)) != SP_MISS)
						return (id);
				}
			}
		}
	}
	return (SP_MISS);
}

__inline int
wi_select(int *dl, int flags, const struct fvec *offp)
{
	int nrecs, pos, lasttry, cubeno, nnodes, ncuts, tries;
	struct wiselstep *ws;

	nnodes = WIDIM_WIDTH * WIDIM_HEIGHT * WIDIM_DEPTH;
	ncuts = 3; /* number of cuts of each dimension */
	tries = log(nnodes) / log(ncuts * ncuts * ncuts);

	if ((ws = calloc(tries, sizeof(*ws))) == NULL)
		err(1, "calloc");

	ws[0].ws_chance = 0;

	ws[0].ws_mag.iv_w = WIDIM_WIDTH;
	ws[0].ws_mag.iv_h = WIDIM_HEIGHT;
	ws[0].ws_mag.iv_d = WIDIM_DEPTH;

	ws[0].ws_off.iv_x = 0;
	ws[0].ws_off.iv_y = 0;
	ws[0].ws_off.iv_z = 0;

	cubeno = SP_MISS;
	for (pos = 0; pos < tries; ) {
		lasttry = (pos == tries - 1);

		sel_begin();
		draw_shadow_wisect(dl, &ws[pos], ncuts, lasttry, offp);
		nrecs = sel_end();
		if (nrecs && (cubeno =
		    sel_process(nrecs, ws[pos].ws_chance, flags)) != SP_MISS) {
		    	if (lasttry)
				goto done;
			cubeno_to_v(cubeno, ncuts, &ws[++pos]);
		} else {
			ws[pos].ws_chance = 0;
			if (--pos < 0)
				goto done;
			ws[pos].ws_chance++;
		}
	}
done:
	free(ws);
	return (cubeno);
}

void
gl_select(int flags)
{
	int i, ret, nrecs;
	struct fvec v;
	int dl[NGEOM];

	switch (stereo_mode) {
	case STM_PASV:
		gl_wid_update();
		break;
	case STM_ACT:
		/* Assume last buffer drawn in. */
		break;
	}

	memset(dl, 0, sizeof(dl));

	sel_begin();
	draw_shadow_panels();
	nrecs = sel_end();
	if (nrecs && (ret = sel_process(nrecs, 0,
	    SPF_2D | flags)) != SP_MISS)
		goto end;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		ret = phys_select(dl, flags);
		break;
	case VM_WIRED:
		WIREP_FOREACH(&v)
			if ((ret = wi_select(dl, flags, &v)) != SP_MISS)
				goto end;
		break;
	case VM_WIREDONE:
		ret = wi_select(dl, flags, &fv_zero);
		break;
	}
end:

	for (i = 0; i < NGEOM; i++)
		if (dl[i])
			glDeleteLists(dl[i], 1);

	st.st_rf |= RF_CAM;

	if (ret == SP_MISS)
		gscb_miss(flags, 0);
}
