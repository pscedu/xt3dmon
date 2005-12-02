void
cluster_getcenter(struct fvec *fv)
{
	struct physdim *pd;
	float *cv;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		pd = LIST_FIRST(&physdims);
		*fv = pd->pd_size;

		switch (pd->pd_spans) {
		case DIM_X:
			cv = &fv->fv_x;
			break;
		case DIM_Y:
			cv = &fv->fv_y;
			break;
		case DIM_Z:
			cv = &fv->fv_z;
			break;
		}
		*cv = pd->pd_mag * (*cv + 2 * pd->pd_space) +
		    (pd->pd_mag - 1) * pd->pd_offset;

		fv->fv_x /= 2.0;
		fv->fv_y /= 2.0;
		fv->fv_z /= 2.0;
		break;
	case VM_WIRED:
		break;
	}
}
