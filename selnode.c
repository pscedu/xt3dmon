/* $Id$ */

void
sel_clear(void)
{
	struct noderange *nr, *next;

	for (nr = LIST_FIRST(&selnodes); nr != LIST_END(&selnodes);
	    nr = next) {
		next = LIST_NEXT(nr, nr_link);
		free(nr);
	}
	LIST_FIRST(&selnodes) = NULL;
}

void
sel_set(struct noderange *nr)
{
	sel_clear();
	sel_union(nr);
}

void
sel_union(struct noderange *nr)
{
	struct noderange *p;

	LIST_FOREACH(&selnodes, p, nr_link) {
		if (nr_collide(nr, nrp)) {

		}
	}
	LIST_INSERT_HEAD(nr);
}

void
sel_intersect(struct noderange *nr)
{
	struct noderange *nrp, *next, *new;
	int len;

	for (nrp = LIST_FIRST(&selnodes); nrp != LIST_END(&selnodes);
	    nrp = next) {
		next = LIST_NEXT(nrp, nr_link);
		if (nr_collide(nr, nrp)) {
			len = min(nr->nr_start.iv_x + nr->nr_dim.iv_w,
			    nrp->nr_start.iv_x + nrp->nr_dim.iv_w) -
			    max(nr->nr_start.iv_x, nrp->nr_start.iv_x);
			if (nr->nr_start.iv_x <= nrp->nr_start.iv_x)
				nrp->nr_start.iv_x += len;
			else if (len < nrp->nr_dim.iv_w) {
				/* Split. */
				if ((new = malloc(sizeof(*new))) == NULL)
					err(1, "malloc");
				new->nr_start.iv_x = nr->nr_start.iv_x + nr->nr_dim.iv_w;
				new->nr_start.iv_y = nr->nr_start.iv_y + nr->nr_dim.iv_h;
				new->nr_start.iv_z = nr->nr_start.iv_z + nr->nr_dim.iv_d;
				new->nr_dim.iv_w = nrp->nr_start.iv_x + nrp->nr_dim.iv_w - new->nr_start.iv_x;
				new->nr_dim.iv_h = nrp->nr_start.iv_y + nrp->nr_dim.iv_h - new->nr_start.iv_y;
				new->nr_dim.iv_d = nrp->nr_start.iv_z + nrp->nr_dim.iv_d - new->nr_start.iv_z;
				LIST_INSERT_AFTER(nrp, new, nr_link);

				len = nrp->nr_dim.iv_w -
				    (nr->nr_start.iv_x - nrp->nr_start.iv_x);
			}
			nrp->nr_dim.iv_w -= len;

			len = min(nr->nr_start.iv_y + nr->nr_dim.iv_h,
			    nrp->nr_start.iv_y + nrp->nr_dim.iv_h) -
			    max(nr->nr_start.iv_y, nrp->nr_start.iv_y);
			if (nr->nr_start.iv_y <= nrp->nr_start.iv_y)
				nrp->nr_start.iv_y += len;
			nrp->nr_dim.iv_h -= len;

			len = min(nr->nr_start.iv_z + nr->nr_dim.iv_d,
			    nrp->nr_start.iv_z + nrp->nr_dim.iv_d) -
			    max(nr->nr_start.iv_z, nrp->nr_start.iv_z);
			if (nr->nr_start.iv_z <= nrp->nr_start.iv_z)
				nrp->nr_start.iv_z += len;
			nrp->nr_dim.iv_d -= len;

			if (nr_size(nrp) == 0) {
				LIST_REMOVE(nrp, nr_link);
				free(nrp);
			}
			if (nr_size(nr) == 0)
				return;
		}
	}
}

sel_walk()
{
}

void
nr_init()
{
}

nr_free()
{
}

int
nr_size(struct noderange *nr)
{
	return (nr->nr_dim.iv_w * nr->nr_dim.iv_h * nr->nr_dim.iv_d);
}

int
nr_collide(struct noderange *a, struct noderange *b)
{
	return (
	    a->nr_start.iv_x < b->nr_start.iv_x + b->nr_dim.iv_w &&
	    a->nr_start.iv_x + a->nr_dim.iv_w > b->nr_start.iv_x &&
	    a->nr_start.iv_y < b->nr_start.iv_y + b->nr_dim.iv_h &&
	    a->nr_start.iv_y + a->nr_dim.iv_h > b->nr_start.iv_y &&
	    a->nr_start.iv_z < b->nr_start.iv_z + b->nr_dim.iv_d &&
	    a->nr_start.iv_z + a->nr_dim.iv_d > b->nr_start.iv_z);
}
