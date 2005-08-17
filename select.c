/* $Id$ */

void
sel_record_begin(void)
{
	GLint viewport[4];
//	float clip;

//	clip = MIN3(WIV_CLIPX, WIV_CLIPY, WIV_CLIPZ);

	glSelectBuffer(sizeof(selbuf) / sizeof(selbuf[0]), selbuf);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glRenderMode(GL_SELECT);
	glInitNames();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPickMatrix(lastu, win_height - lastv, 1, 1, viewport);
	/* XXX wrong */
	gluPerspective(FOVY, ASPECT, 0.1, clip);
	glMatrixMode(GL_MODELVIEW);
}

#define SBI_LEN		0
#define SBI_UDST	1
#define SBI_VDST	2
#define SBI_NAMEOFF	3

int
sel_record_process(GLint nrecs, int *done)
{
	GLuint minu, minv, *p, name;
	int i, found, nametype;
	struct panel *pl;
	struct node *n;
	GLuint lastlen;
	GLuint origname = -1;

	*done = 1;
	name = 0; /* gcc */
	found = nametype = 0;
	minu = minv = UINT_MAX;
	/* XXX:  sanity-check nrecs? */
	for (i = 0, p = (GLuint *)selbuf; i < nrecs;
	    i++, lastlen = p[SBI_LEN], p += 3 + p[SBI_LEN]) {
		/*
		 * Each record consists of the following:
		 *	- the number of names in this stack frame,
		 *	- the minimum and maximum depths of all
		 *	  vertices hit since the previous event, and
		 *	- stack contents, bottom first
		 */
		if (p[SBI_LEN] != 1)
			continue;
		if (p[SBI_UDST] <= minu) {
			minu = p[SBI_UDST];
			if (p[SBI_VDST] < minv) {
				/*
				 * XXX: hack to allow 2D objects to
				 * be properly selected.
				 */
				glnametype(p[SBI_NAMEOFF], &origname,
				    &nametype);
				if (nametype == GNAMT_PANEL) {
					if ((pl =
					    panel_for_id(origname)) == NULL)
						continue;
					if (lastu < pl->p_u ||
					    lastu > pl->p_u + pl->p_w ||
					    lastv < win_height - pl->p_v ||
					    lastv > win_height - pl->p_v + pl->p_h)
						continue;
				}
				name = p[SBI_NAMEOFF];
				minv = p[SBI_VDST];
				found = 1;
			}
		}

	}
	if (found) {
		glnametype(name, &origname,
		    &nametype);
		switch (nametype) {
#if 0
		case GNAMT_NODE:
			if ((n = node_for_nid(origname)) != NULL) {
				switch (spkey) {
				case GLUT_ACTIVE_SHIFT:
					sel_add(n);
					break;
				case GLUT_ACTIVE_CTRL:
					sel_del(n);
					break;
				case 0:
					sel_set(n);
					break;
				}
				if (SLIST_EMPTY(&selnodes))
					panel_hide(PANEL_NINFO);
				else
					panel_show(PANEL_NINFO);
			}
			break;
#endif
		case GNAMT_PANEL:
			if (spkey == GLUT_ACTIVE_CTRL) {
				glutMotionFunc(m_activeh_panel);
				/* GL giving us crap. */
				if ((panel_mobile =
				    panel_for_id(origname)) != NULL)
					panel_mobile->p_opts |= POPT_MOBILE;
			}
			break;

		/* anything special needed here? XXX */
		case GNAMT_ROW: printf("row selected: %d\n", origname); *done = 0; break;
		case GNAMT_CAB: printf("cabnet selected: %d\n", origname); *done = 0; break;
		case GNAMT_CAG: printf("cage selected: %d\n", origname); *done = 0; break;
		case GNAMT_MOD: printf("module: %d\n", origname); *done = 0; break;
		case GNAMT_NODE: printf("node: %d\n", origname); break;
		}
	}

	return origname;
}

int
sel_record_end(int *done)
{
	int nrecs;
	int ret = -1;

	*done = 1;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glFlush();

	if ((nrecs = glRenderMode(GL_RENDER)) != 0) {
		ret = sel_record_process(nrecs, done);
	}

	if (done) {
		drawh = drawh_old;
		glutDisplayFunc(drawh);
	}
	/* st.st_rf |= RF_CAM; */
	rebuild(RF_CAM);

	return ret;
}

void
gn_mkname(int id, void (*cb)(unsigned int))
{
	struct glname *gn;

	gn = getobj(&id, glname_list);
	gt->gt_id = id;
	gt->gt_cb = cb;
}
