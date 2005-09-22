/* $Id$ */

#include "compat.h"

#include <math.h>

#include "mon.h"

struct wiselstep {
	int			ws_chance;
	struct ivec		ws_off;
	struct ivec		ws_mag;
};

void
draw_shadow_rows(void)
{
	struct fvec dim, v;
	int r;

	v.fv_x = NODESPACE;
	v.fv_y = NODESPACE;
	v.fv_z = NODESPACE;

	/* XXX Width is a little off add NODEWIDTH? */
	dim.fv_w = ROWWIDTH; // + NODEWIDTH;
	dim.fv_h = CABHEIGHT;
	dim.fv_d = ROWDEPTH + NODESHIFT;

	for (r = 0; r < NROWS; r++, v.fv_z += ROWSPACE + ROWDEPTH) {
		glPushMatrix();
		glTranslatef(v.fv_x, v.fv_y, v.fv_z);
		glPushName(gsn_get(r, NULL, 0));
		draw_box_filled(&dim, &fill_black);
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
	v.fv_z = NODESPACE + pc->pc_r * (ROWSPACE + ROWDEPTH);

	dim.fv_w = CABWIDTH + NODEWIDTH;
	dim.fv_h = CABHEIGHT;
	dim.fv_d = ROWDEPTH + NODESHIFT;

	for (cb = 0; cb < NCABS; cb++, v.fv_x += CABWIDTH + CABSPACE) {
		glPushMatrix();
		glTranslatef(v.fv_x, v.fv_y, v.fv_z);
		glPushName(gsn_get(cb, NULL, 0));
		draw_box_filled(&dim, &fill_black);
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
	v.fv_z = NODESPACE + pc->pc_r * (ROWSPACE + ROWDEPTH);

	dim.fv_w = CABWIDTH + NODEWIDTH;
	dim.fv_h = CAGEHEIGHT;
	dim.fv_d = ROWDEPTH + NODESHIFT;

	for (cg = 0; cg < NCAGES; cg++, v.fv_y += CAGEHEIGHT + CAGESPACE) {
		glPushMatrix();
		glTranslatef(v.fv_x, v.fv_y, v.fv_z);
		glPushName(gsn_get(cg, NULL, 0));
		draw_box_filled(&dim, &fill_black);
		glPopName();
		glPopMatrix();
	}
}

void
draw_shadow_mods(const struct physcoord *pc)
{
	struct fvec dim, v;
	int m;

	/* Account for the cabinet. */
	v.fv_x = NODESPACE + pc->pc_cb * (CABSPACE + CABWIDTH);
	v.fv_y = NODESPACE + pc->pc_cg * (CAGESPACE + CAGEHEIGHT);
	v.fv_z = NODESPACE + pc->pc_r * (ROWSPACE + ROWDEPTH);

	dim.fv_w = MODWIDTH + NODEWIDTH;
	dim.fv_h = NODEHEIGHT;
	dim.fv_d = NODEDEPTH * 2 + NODESPACE;

	for (m = 0; m < NMODS; m++, v.fv_x += MODWIDTH + MODSPACE) {
		glPushMatrix();
		glPushName(gsn_get(m, NULL, 0));

		/* Draw the rows individually. */
		glTranslatef(v.fv_x, v.fv_y, v.fv_z);
		draw_box_filled(&dim, &fill_black);
		glTranslatef(0, NODESPACE + NODEHEIGHT, NODESHIFT);
		draw_box_filled(&dim, &fill_black);

		glPopName();
		glPopMatrix();
	}
}

void
draw_shadow_nodes(const struct physcoord *pc)
{
	struct fvec dim, v, nv;
	struct node *node;
	int n;

	v.fv_x = NODESPACE + pc->pc_cb * (CABSPACE + CABWIDTH);
	v.fv_y = NODESPACE + pc->pc_cg * (CAGESPACE + CAGEHEIGHT);
	v.fv_z = NODESPACE + pc->pc_r * (ROWSPACE + ROWDEPTH);

	/* Account for the module. */
	v.fv_x += (MODWIDTH + MODSPACE) * pc->pc_m;

	dim.fv_w = NODEWIDTH;
	dim.fv_h = NODEHEIGHT;
	dim.fv_d = NODEDEPTH;

	/* row 1 is offset -> z, col 1 is offset z too */
	for (n = 0; n < NNODES; n++) {
		nv = v;
		node_adjmodpos(n, &nv);

		node = &nodes[pc->pc_r][pc->pc_cb][pc->pc_cg][pc->pc_m][n];
		if (node->n_flags & NF_HIDE)
			continue;

		glPushMatrix();
		glTranslatef(nv.fv_x, nv.fv_y, nv.fv_z);
		glPushName(gsn_get(node->n_nid, gscb_node, 0));
		draw_box_filled(&dim, &fill_black);
		glPopName();
		glPopMatrix();
	}
}

static int init = 0;

void
draw_shadow_wisect(struct wiselstep *ws, int cuts)
{
	struct fvec onv, nv, dim, adj;
	struct ivec mag, len, *offp;
	int cubeno;

	offp = &ws->ws_off;

	len = mag = ws->ws_mag;
	len.iv_x /= cuts;
	len.iv_y /= cuts;
	len.iv_z /= cuts;

#if 0
	if (len.iv_x == 0 ||
	    len.iv_y == 0 ||
	    len.iv_z == 0)
		errx(1, "internal");
#endif

	nv.fv_x = offp->iv_x * st.st_winsp.iv_x;
	nv.fv_y = offp->iv_y * st.st_winsp.iv_y;
	nv.fv_z = offp->iv_z * st.st_winsp.iv_z;
	onv = nv;

	adj.fv_x = len.iv_x * st.st_winsp.iv_x;
	adj.fv_y = len.iv_y * st.st_winsp.iv_y;
	adj.fv_z = len.iv_z * st.st_winsp.iv_z;

if (init) {
printf("   -> drawing\n");
	if (cluster_dl)
		glDeleteLists(cluster_dl, 1);
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);
}


printf("   (%d,%d) (%d,%d) (%d,%d)\n",
 mag.iv_x, len.iv_x,
 mag.iv_y, len.iv_y,
 mag.iv_z, len.iv_z);

	cubeno = 0;
	for (; mag.iv_x > len.iv_x; nv.fv_x += adj.fv_x) {
		mag.iv_x -= len.iv_x;
		if (mag.iv_x < len.iv_x)
			len.iv_w += mag.iv_x;
		dim.fv_w = (len.iv_x - 1) * st.st_winsp.iv_x + NODEWIDTH;

		/*
		 * Restore magnitude/length as it may have been
		 * modified by the last loop iteration below.
		 *
		 * The same applies below to 'z'.
		 */
		mag.iv_y = ws->ws_mag.iv_y;
		len.iv_y = mag.iv_y / cuts;
		for (; mag.iv_y > len.iv_y; nv.fv_y += adj.fv_y) {
			mag.iv_y -= len.iv_y;
			if (mag.iv_y < len.iv_y)
				len.iv_h += mag.iv_y;
			dim.fv_h = (len.iv_y - 1) * st.st_winsp.iv_y + NODEHEIGHT;

			mag.iv_z = ws->ws_mag.iv_z;
			len.iv_z = mag.iv_z / cuts;
			for (; mag.iv_z > len.iv_z; nv.fv_z += adj.fv_z) {
				mag.iv_z -= len.iv_z;
				if (mag.iv_z < len.iv_z)
					len.iv_d += mag.iv_z;
				dim.fv_d = (len.iv_z - 1) * st.st_winsp.iv_z + NODEDEPTH;
printf("c"); fflush(stdout);
				glPushMatrix();
				glTranslatef(nv.fv_x, nv.fv_y, nv.fv_z);
				glPushName(gsn_get(cubeno++, NULL, 0));
				draw_box_filled(&dim, cubeno % 2 ? &fill_light_blue : &fill_black);
				glPopName();
				glPopMatrix();
			}
			nv.fv_z = onv.fv_z;
		}
		nv.fv_y = onv.fv_y;
	}
printf("\n");

if (init)
	glEndList();
init = 1;
}

static void
cubeno_to_v(int cubeno, int cuts, struct wiselstep *ws)
{
	struct ivec iv, vcuts, len, *magp;
	struct wiselstep *pws;

	pws = ws - 1;
	magp = &pws->ws_mag;
	ws->ws_off = pws->ws_off;

	len = *magp;
	len.iv_x /= cuts;
	len.iv_y /= cuts;
	len.iv_z /= cuts;

	vcuts.iv_x = howmany(magp->iv_x, magp->iv_x / cuts);
	vcuts.iv_y = howmany(magp->iv_y, magp->iv_y / cuts);
	vcuts.iv_z = howmany(magp->iv_z, magp->iv_z / cuts);

	iv.iv_x = cubeno / vcuts.iv_x;
	cubeno %= vcuts.iv_x;

	iv.iv_y = cubeno / vcuts.iv_y;
	cubeno %= vcuts.iv_y;

	iv.iv_z = cubeno;

	ws->ws_off.iv_x += iv.iv_x;
	ws->ws_off.iv_y += iv.iv_y;
	ws->ws_off.iv_z += iv.iv_z;

	ws->ws_mag.iv_x = MIN(len.iv_x, magp->iv_x - iv.iv_x);
	ws->ws_mag.iv_y = MIN(len.iv_y, magp->iv_y - iv.iv_y);
	ws->ws_mag.iv_z = MIN(len.iv_y, magp->iv_z - iv.iv_z);

printf("new x mag is %d\n", ws->ws_mag.iv_x);
}

void
draw_shadow_winodes(struct ivec *off, struct ivec *mag)
{
	struct node *node;
	struct fvec dim;
	struct ivec iv;

	dim.fv_x = NODEWIDTH;
	dim.fv_y = NODEHEIGHT;
	dim.fv_z = NODEDEPTH;

	for (iv.iv_x = off->iv_x; iv.iv_x < off->iv_x + mag->iv_x; iv.iv_x++)
		for (iv.iv_y = off->iv_y; iv.iv_y < off->iv_y + mag->iv_y; iv.iv_y++)
			for (iv.iv_z = off->iv_z; iv.iv_z < off->iv_z + mag->iv_z; iv.iv_z++) {
				node = wimap[iv.iv_x][iv.iv_y][iv.iv_z];

				glPushMatrix();
				glTranslatef(node->n_v->fv_x,
				    node->n_v->fv_y,
				    node->n_v->fv_z);
				glPushName(gsn_get(node->n_nid, gscb_node, 0));
				draw_box_filled(&dim, &fill_black);
				glPopName();
				glPopMatrix();
			}
}

void
drawh_select(void)
{
	struct physcoord pc, chance;
	int nrecs;

	sel_begin();
	draw_shadow_panels();
	nrecs = sel_end();
	if (nrecs && sel_process(nrecs, 0, SPF_2D) != SP_MISS)
		goto end;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		for (chance.pc_r = 0; chance.pc_r < NROWS; chance.pc_r++) {
			sel_begin();
			draw_shadow_rows();
			nrecs = sel_end();
			if (nrecs == 0 ||
			    (pc.pc_r = sel_process(nrecs, chance.pc_r, 0)) == SP_MISS)
				break;
			for (chance.pc_cb = 0; chance.pc_cb < NCABS; chance.pc_cb++) {
				sel_begin();
				draw_shadow_cabs(&pc);
				nrecs = sel_end();
				if (nrecs == 0 || (pc.pc_cb =
				    sel_process(nrecs, chance.pc_cb, 0)) == SP_MISS)
					break;
				for (chance.pc_cg = 0; chance.pc_cg < NCAGES; chance.pc_cg++) {
					sel_begin();
					draw_shadow_cages(&pc);
					nrecs = sel_end();
					if (nrecs == 0 || (pc.pc_cg =
					    sel_process(nrecs, chance.pc_cg, 0)) == SP_MISS)
						break;
					for (chance.pc_m = 0; chance.pc_m < NMODS; chance.pc_m++) {
						sel_begin();
						draw_shadow_mods(&pc);
						nrecs = sel_end();
						if (nrecs == 0 || (pc.pc_m =
						    sel_process(nrecs, chance.pc_m, 0)) == SP_MISS)
							break;

						sel_begin();
						draw_shadow_nodes(&pc);
						nrecs = sel_end();
						if (nrecs && sel_process(nrecs, 0, 0) != SP_MISS)
							goto end;
					}
				}
			}
		}
		break;
	case VM_WIRED:
	case VM_WIREDONE: {
		int pos, cubeno, rem, ncuts, tries;
		struct wiselstep *ws;
init = 0;
		rem = WIDIM_WIDTH * WIDIM_HEIGHT * WIDIM_DEPTH;
		ncuts = 3; /* number of cuts of each dimension */
		tries = log(rem) / log(ncuts);
		if ((ws = calloc(tries, sizeof(*ws))) == NULL)
			err(1, "calloc");

		ws[0].ws_chance = 0;

		ws[0].ws_mag.iv_w = WIDIM_WIDTH;
		ws[0].ws_mag.iv_h = WIDIM_HEIGHT;
		ws[0].ws_mag.iv_d = WIDIM_DEPTH;

		ws[0].ws_off.iv_x = 0;
		ws[0].ws_off.iv_y = 0;
		ws[0].ws_off.iv_z = 0;

printf("\ntries: %d\n", tries);
		for (pos = 0; pos < tries; ) {
printf(" %d\n", pos);
			sel_begin();
printf("  begin\n");
			draw_shadow_wisect(&ws[pos], ncuts);
printf("  draw\n");
			nrecs = sel_end();
printf("  end\n");
			if (nrecs && (cubeno =
			    sel_process(nrecs, ws[pos].ws_chance, 0)) != SP_MISS) {
				cubeno_to_v(cubeno, ncuts, &ws[++pos]);
printf("  hit cube %d\n", cubeno);
				if (ws[pos].ws_mag.iv_x == 1 ||
				    ws[pos].ws_mag.iv_y == 1 ||
				    ws[pos].ws_mag.iv_z == 1)
					break;
			} else {
printf("  fail\n");
				ws[pos].ws_chance = 0;
				if (--pos < 0)
					goto free;
				ws[pos].ws_chance++;
			}
		}
printf("@@ READY\n");
free:
		free(ws);
		break;
	    }
	}
end:
	drawh = drawh_old;
	glutDisplayFunc(drawh);
	st.st_rf |= RF_CAM;
}
