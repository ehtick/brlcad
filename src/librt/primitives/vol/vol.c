/*                           V O L . C
 * BRL-CAD
 *
 * Copyright (c) 1989-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup primitives */
/** @{ */
/** @file primitives/vol/vol.c
 *
 * Intersect a ray with a 3-D volume.  The volume is described as a
 * concatenation of bw(5) files.
 *
 */
/** @} */

#include "common.h"

#include <stddef.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "bio.h"

#include "bu/parallel.h"
#include "vmath.h"
#include "rt/db4.h"
#include "nmg.h"
#include "rt/geom.h"
#include "raytrace.h"

#include "../fixpt.h"


/*
  NOTES:
  Changed small to small1 for win32 compatibility
*/


struct rt_vol_specific {
    struct rt_vol_internal vol_i;
    vect_t vol_xnorm;	/* local +X norm in model coords */
    vect_t vol_ynorm;
    vect_t vol_znorm;
    mat_t vol_mat;	/* model to ideal space */
    vect_t vol_origin;	/* local coords of grid origin (0, 0, 0) for now */
    vect_t vol_large;	/* local coords of XYZ max */
};
#define VOL_NULL ((struct rt_vol_specific *)0)

#define VOL_O(m) bu_offsetof(struct rt_vol_internal, m)

const struct bu_structparse rt_vol_parse[] = {
    {"%s", RT_VOL_NAME_LEN, "file", bu_offsetof(struct rt_vol_internal, name), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%s", RT_VOL_NAME_LEN, "name", bu_offsetof(struct rt_vol_internal, name), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%c", 1, "src",	VOL_O(datasrc),	BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%d", 1, "w", VOL_O(xdim), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%d", 1, "n", VOL_O(ydim), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%d", 1, "d", VOL_O(zdim), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%d", 1, "lo", VOL_O(lo), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%d", 1, "hi", VOL_O(hi), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%f", ELEMENTS_PER_VECT, "size", bu_offsetof(struct rt_vol_internal, cellsize), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"%f", 16, "mat", bu_offsetof(struct rt_vol_internal, mat), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    {"", 0, (char *)0, 0, BU_STRUCTPARSE_FUNC_NULL, NULL, NULL }
};


extern void rt_vol_plate(point_t a, point_t b, point_t c, point_t d,
			 mat_t mat, struct bu_list *vlfree, struct bu_list *vhead, struct rt_vol_internal *vip);
extern int rt_retrieve_binunif(struct rt_db_internal *intern, const struct db_i *dbip, const char *name);
extern int rt_binunif_describe(struct bu_vls  *str, const struct rt_db_internal *ip, int verbose, double mm2local);
/*
 * Codes to represent surface normals.
 * In a bitmap, there are only 4 possible normals.
 * With this code, reverse the sign to reverse the direction.
 * As always, the normal is expected to point outwards.
 */
#define NORM_ZPOS 3
#define NORM_YPOS 2
#define NORM_XPOS 1
#define NORM_XNEG (-1)
#define NORM_YNEG (-2)
#define NORM_ZNEG (-3)

/*
 * Regular bit addressing is used:  (0..W-1, 0..N-1),
 * but the bitmap is stored with two cells of zeros all around,
 * so permissible subscripts run (-2..W+1, -2..N+1).
 * This eliminates special-case code for the boundary conditions.
 */
#define VOL_XWIDEN 2
#define VOL_YWIDEN 2
#define VOL_ZWIDEN 2
#define VOLIDX(_xdim, _ydim, _xx, _yy, _zz) \
	(((_zz)+VOL_ZWIDEN) \
	 * ((_ydim) + VOL_YWIDEN*2) \
	 + ((_yy)+VOL_YWIDEN)) \
	* ((_xdim) + VOL_XWIDEN*2) \
	+ (_xx)+VOL_XWIDEN
#define VOLMAP(_map, _xdim, _ydim, _xx, _yy, _zz) (_map)[ VOLIDX(_xdim, _ydim, _xx, _yy, _zz) ]
#define VOL(_vip, _xx, _yy, _zz) VOLMAP((_vip)->map, (_vip)->xdim, (_vip)->ydim, _xx, _yy, _zz)

#define OK(_vip, _v)	((_v) >= (_vip)->lo && (_v) <= (_vip)->hi)

static int rt_vol_normtab[3] = { NORM_XPOS, NORM_YPOS, NORM_ZPOS };

/**
 * Transform the ray into local coordinates of the volume ("ideal space").
 * Step through the 3-D array, in local coordinates.
 * Return intersection segments.
 *
 */
int
rt_vol_shot(struct soltab *stp, register struct xray *rp, struct application *ap, struct seg *seghead)
{
    register struct rt_vol_specific *volp =
	(struct rt_vol_specific *)stp->st_specific;
    vect_t invdir;
    double t0;	/* in point of cell */
    double t1;	/* out point of cell */
    double tmax;	/* out point of entire grid */
    vect_t t;	/* next t value for XYZ cell plane intersect */
    vect_t delta;	/* spacing of XYZ cell planes along ray */
    int igrid[3];/* Grid cell coordinates of cell (integerized) */
    vect_t P;	/* hit point */
    int inside;	/* inside/outside a solid flag */
    int in_axis = -INT_MAX;
    int axis_set = 0;
    int out_axis = -INT_MAX;
    int j;
    struct xray ideal_ray;

    /* Transform actual ray into ideal space at origin in X-Y plane */
    MAT4X3PNT(ideal_ray.r_pt, volp->vol_mat, rp->r_pt);
    MAT4X3VEC(ideal_ray.r_dir, volp->vol_mat, rp->r_dir);
    rp = &ideal_ray;	/* XXX */

    /* Compute the inverse of the direction cosines */
    if (!ZERO(rp->r_dir[X])) {
	invdir[X] = 1.0/rp->r_dir[X];
    } else {
	invdir[X] = INFINITY;
	rp->r_dir[X] = 0.0;
    }
    if (!ZERO(rp->r_dir[Y])) {
	invdir[Y] = 1.0/rp->r_dir[Y];
    } else {
	invdir[Y] = INFINITY;
	rp->r_dir[Y] = 0.0;
    }
    if (!ZERO(rp->r_dir[Z])) {
	invdir[Z] = 1.0/rp->r_dir[Z];
    } else {
	invdir[Z] = INFINITY;
	rp->r_dir[Z] = 0.0;
    }

    /* intersect ray with ideal grid rpp */
    VSETALL(P, 0);
    if (! rt_in_rpp(rp, invdir, P, volp->vol_large))
	return 0;	/* MISS */
    VJOIN1(P, rp->r_pt, rp->r_min, rp->r_dir);	/* P is hit point */
    if (RT_G_DEBUG&RT_DEBUG_VOL)VPRINT("vol_large", volp->vol_large);
    if (RT_G_DEBUG&RT_DEBUG_VOL)VPRINT("vol_origin", volp->vol_origin);
    if (RT_G_DEBUG&RT_DEBUG_VOL)VPRINT("r_pt", rp->r_pt);
    if (RT_G_DEBUG&RT_DEBUG_VOL)VPRINT("P", P);
    if (RT_G_DEBUG&RT_DEBUG_VOL)VPRINT("cellsize", volp->vol_i.cellsize);
    t0 = rp->r_min;
    tmax = rp->r_max;
    if (RT_G_DEBUG&RT_DEBUG_VOL)bu_log("[shoot: r_min=%g, r_max=%g]\n", rp->r_min, rp->r_max);

    /* find grid cell where ray first hits ideal space bounding RPP */
    igrid[X] = (P[X] - volp->vol_origin[X]) / volp->vol_i.cellsize[X];
    igrid[Y] = (P[Y] - volp->vol_origin[Y]) / volp->vol_i.cellsize[Y];
    igrid[Z] = (P[Z] - volp->vol_origin[Z]) / volp->vol_i.cellsize[Z];
    if (igrid[X] < 0) {
	igrid[X] = 0;
    } else if ((size_t)igrid[X] >= volp->vol_i.xdim) {
	igrid[X] = volp->vol_i.xdim-1;
    }
    if (igrid[Y] < 0) {
	igrid[Y] = 0;
    } else if ((size_t)igrid[Y] >= volp->vol_i.ydim) {
	igrid[Y] = volp->vol_i.ydim-1;
    }
    if (igrid[Z] < 0) {
	igrid[Z] = 0;
    } else if ((size_t)igrid[Z] >= volp->vol_i.zdim) {
	igrid[Z] = volp->vol_i.zdim-1;
    }
    if (RT_G_DEBUG&RT_DEBUG_VOL)bu_log("igrid=(%d, %d, %d)\n", igrid[X], igrid[Y], igrid[Z]);

    /* X setup */
    if (ZERO(rp->r_dir[X])) {
	t[X] = INFINITY;
	delta[X] = 0;
    } else {
	j = igrid[X];
	if (rp->r_dir[X] < 0) j++;
	t[X] = (volp->vol_origin[X] + j*volp->vol_i.cellsize[X] -
		rp->r_pt[X]) * invdir[X];
	delta[X] = volp->vol_i.cellsize[X] * fabs(invdir[X]);

	/* face of entry into first cell -- max initial t value */
	t0 = t[X];
	in_axis = X;
	axis_set = 1;
    }
    /* Y setup */
    if (ZERO(rp->r_dir[Y])) {
	t[Y] = INFINITY;
	delta[Y] = 0;
    } else {
	j = igrid[Y];
	if (rp->r_dir[Y] < 0) j++;
	t[Y] = (volp->vol_origin[Y] + j*volp->vol_i.cellsize[Y] -
		rp->r_pt[Y]) * invdir[Y];
	delta[Y] = volp->vol_i.cellsize[Y] * fabs(invdir[Y]);

	/* face of entry into first cell -- max initial t value */
	if (axis_set) {
	    if (t[Y] > t0) {
		t0 = t[Y];
		in_axis = Y;
	    }
	}
	else {
	    t0 = t[Y];
	    in_axis = Y;
	    axis_set = 1;
	}
    }
    /* Z setup */
    if (ZERO(rp->r_dir[Z])) {
	t[Z] = INFINITY;
	delta[Z] = 0;
    } else {
	j = igrid[Z];
	if (rp->r_dir[Z] < 0) j++;
	t[Z] = (volp->vol_origin[Z] + j*volp->vol_i.cellsize[Z] -
		rp->r_pt[Z]) * invdir[Z];
	delta[Z] = volp->vol_i.cellsize[Z] * fabs(invdir[Z]);

	/* face of entry into first cell -- max initial t value */
	if (axis_set) {
	    if (t[Z] > t0) {
		t0 = t[Z];
		in_axis = Z;
	    }
	}
	else {
	    t0 = t[Z];
	    in_axis = Z;
	    axis_set = 1;
	}
    }

    /* The delta[] elements *must* be positive, as t must increase */
    if (RT_G_DEBUG&RT_DEBUG_VOL)bu_log("t[X] = %g, delta[X] = %g\n", t[X], delta[X]);
    if (RT_G_DEBUG&RT_DEBUG_VOL)bu_log("t[Y] = %g, delta[Y] = %g\n", t[Y], delta[Y]);
    if (RT_G_DEBUG&RT_DEBUG_VOL)bu_log("t[Z] = %g, delta[Z] = %g\n", t[Z], delta[Z]);

    if (!axis_set) bu_log("ERROR vol: no valid entry face found\n");

    if (RT_G_DEBUG&RT_DEBUG_VOL && in_axis != -INT_MAX)
	bu_log("Entry axis is %s, t0=%g\n", in_axis==X ? "X" : (in_axis==Y?"Y":"Z"), t0);

    /* Advance to next exits */
    t[X] += delta[X];
    t[Y] += delta[Y];
    t[Z] += delta[Z];

    /* Ensure that next exit is after first entrance */
    if (t[X] < t0) {
	bu_log("*** advancing t[X]\n");
	t[X] += delta[X];
    }
    if (t[Y] < t0) {
	bu_log("*** advancing t[Y]\n");
	t[Y] += delta[Y];
    }
    if (t[Z] < t0) {
	bu_log("*** advancing t[Z]\n");
	t[Z] += delta[Z];
    }
    if (RT_G_DEBUG&RT_DEBUG_VOL) VPRINT("Exit t[]", t);

    inside = 0;

    while (t0 < tmax) {
	int val;
	struct seg *segp;

	/* find minimum exit t value */
	if (t[X] < t[Y]) {
	    if (t[Z] < t[X]) {
		out_axis = Z;
		t1 = t[Z];
	    } else {
		out_axis = X;
		t1 = t[X];
	    }
	} else {
	    if (t[Z] < t[Y]) {
		out_axis = Z;
		t1 = t[Z];
	    } else {
		out_axis = Y;
		t1 = t[Y];
	    }
	}

	/* Ray passes through cell igrid[XY] from t0 to t1 */
	val = VOL(&volp->vol_i, igrid[X], igrid[Y], igrid[Z]);
	if (RT_G_DEBUG&RT_DEBUG_VOL)bu_log("igrid [%d %d %d] from %g to %g, val=%d\n",
					igrid[X], igrid[Y], igrid[Z],
					t0, t1, val);
	if (RT_G_DEBUG&RT_DEBUG_VOL)bu_log("Exit axis is %s, t[]=(%g, %g, %g)\n",
					out_axis==X ? "X" : (out_axis==Y?"Y":"Z"),
					t[X], t[Y], t[Z]);

	if (t1 <= t0) bu_log("ERROR vol t1=%g < t0=%g\n", t1, t0);
	if (!inside) {
	    if (OK(&volp->vol_i, (size_t)val)) {
		/* Handle the transition from vacuum to solid */
		/* Start of segment (entering a full voxel) */
		inside = 1;

		RT_GET_SEG(segp, ap->a_resource);
		segp->seg_stp = stp;
		segp->seg_in.hit_dist = t0;

		/* Compute entry normal */
		if (rp->r_dir[in_axis] < 0) {
		    /* Go left, entry norm goes right */
		    segp->seg_in.hit_surfno =
			rt_vol_normtab[in_axis];
		} else {
		    /* go right, entry norm goes left */
		    segp->seg_in.hit_surfno =
			(-rt_vol_normtab[in_axis]);
		}
		BU_LIST_INSERT(&(seghead->l), &(segp->l));
		if (RT_G_DEBUG&RT_DEBUG_VOL) bu_log("START t=%g, surfno=%d\n",
						 t0, segp->seg_in.hit_surfno);
	    } else {
		/* Do nothing, marching through void */
	    }
	} else {
	    if (OK(&volp->vol_i, (size_t)val)) {
		/* Do nothing, marching through solid */
	    } else {
		register struct seg *tail;
		/* End of segment (now in an empty voxel) */
		/* Handle transition from solid to vacuum */
		inside = 0;

		tail = BU_LIST_LAST(seg, &(seghead->l));
		tail->seg_out.hit_dist = t0;

		/* Compute exit normal */
		if (rp->r_dir[in_axis] < 0) {
		    /* Go left, exit normal goes left */
		    tail->seg_out.hit_surfno =
			(-rt_vol_normtab[in_axis]);
		} else {
		    /* go right, exit norm goes right */
		    tail->seg_out.hit_surfno =
			rt_vol_normtab[in_axis];
		}
		if (RT_G_DEBUG&RT_DEBUG_VOL) bu_log("END t=%g, surfno=%d\n",
						 t0, tail->seg_out.hit_surfno);
	    }
	}

	/* Take next step */
	t0 = t1;
	in_axis = out_axis;
	t[out_axis] += delta[out_axis];
	if (rp->r_dir[out_axis] > 0) {
	    igrid[out_axis]++;
	} else {
	    igrid[out_axis]--;
	}
    }

    if (inside) {
	register struct seg *tail;

	/* Close off the final segment */
	tail = BU_LIST_LAST(seg, &(seghead->l));
	tail->seg_out.hit_dist = tmax;

	/* Compute exit normal.  Previous out_axis is now in_axis */
	if (rp->r_dir[in_axis] < 0) {
	    /* Go left, exit normal goes left */
	    tail->seg_out.hit_surfno = (-rt_vol_normtab[in_axis]);
	} else {
	    /* go right, exit norm goes right */
	    tail->seg_out.hit_surfno = rt_vol_normtab[in_axis];
	}
	if (RT_G_DEBUG&RT_DEBUG_VOL) bu_log("closed END t=%g, surfno=%d\n",
					 tmax, tail->seg_out.hit_surfno);
    }

    if (BU_LIST_IS_EMPTY(&(seghead->l)))
	return 0;
    return 2;
}


/**
 * Read in the information from the string solid record.
 * Then, as a service to the application, read in the bitmap
 * and set up some of the associated internal variables.
 */
int
rt_vol_import4(struct rt_db_internal *ip, const struct bu_external *ep, const fastf_t *mat, const struct db_i *dbip)
{
    union record *rp;
    register struct rt_vol_internal *vip;
    struct bu_vls str = BU_VLS_INIT_ZERO;
    FILE *fp;
    int nbytes;
    size_t y;
    size_t z;
    mat_t tmat;
    size_t ret;

    if (dbip) RT_CK_DBI(dbip);

    BU_CK_EXTERNAL(ep);
    rp = (union record *)ep->ext_buf;
    if (rp->u_id != DBID_STRSOL) {
	bu_log("rt_vol_import4: defective strsol record\n");
	return -1;
    }

    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_VOL;
    ip->idb_meth = &OBJ[ID_VOL];
    BU_ALLOC(ip->idb_ptr, struct rt_vol_internal);

    vip = (struct rt_vol_internal *)ip->idb_ptr;
    vip->magic = RT_VOL_INTERNAL_MAGIC;

    /* Establish defaults */
    MAT_IDN(vip->mat);
    vip->lo = 0;
    vip->hi = 255;

    /* Default VOL cell size in ideal coordinates is one unit/cell */
    VSETALL(vip->cellsize, 1);

    /* default source is file for compatibility with old VOL */
    vip->datasrc = RT_VOL_SRC_FILE;

    bu_vls_strcpy(&str, rp->ss.ss_args);
    if (bu_struct_parse(&str, rt_vol_parse, (char *)vip, NULL) < 0) {
	bu_vls_free(&str);
	return -2;
    }
    bu_vls_free(&str);

    /* Check for reasonable values */
    if (vip->name[0] == '\0' || vip->xdim < 1 ||
	vip->ydim < 1 || vip->zdim < 1 || vip->mat[15] <= 0.0 ||
	vip->hi > 255) {
	bu_struct_print("Unreasonable VOL parameters", rt_vol_parse,
			(char *)vip);
	return -1;
    }

    /* Apply any modeling transforms to get final matrix */
    if (mat == NULL) mat = bn_mat_identity;
    bn_mat_mul(tmat, mat, vip->mat);
    MAT_COPY(vip->mat, tmat);

    /* Get bit map from .bw(5) file */
    nbytes = (vip->xdim+VOL_XWIDEN*2)*
	(vip->ydim+VOL_YWIDEN*2)*
	(vip->zdim+VOL_ZWIDEN*2);
    vip->map = (unsigned char *)bu_calloc(1, nbytes, "vol_import4 bitmap");

    bu_semaphore_acquire(BU_SEM_SYSCALL);		/* lock */
    if ((fp = fopen(vip->name, "rb")) == NULL) {
	perror(vip->name);
	bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */
	return -1;
    }
    bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */

    /* Because of in-memory padding, read each scanline separately */
    for (z = 0; z < vip->zdim; z++) {
	for (y = 0; y < vip->ydim; y++) {
	    void *data = &VOLMAP(vip->map, vip->xdim, vip->ydim, 0, y, z);
	    bu_semaphore_acquire(BU_SEM_SYSCALL);		/* lock */
	    ret = fread(data, vip->xdim, 1, fp); /* res_syscall */
	    bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */
	    if (ret < 1) {
		bu_log("rt_vol_import4(%s): Unable to read whole VOL, y=%zu, z=%zu\n",
		       vip->name, y, z);
		bu_semaphore_acquire(BU_SEM_SYSCALL);		/* lock */
		fclose(fp);
		bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */
		return -1;
	    }
	}
    }
    bu_semaphore_acquire(BU_SEM_SYSCALL);		/* lock */
    fclose(fp);
    bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */
    return 0;
}


/**
 * The name will be added by the caller.
 */
int
rt_vol_export4(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_vol_internal *vip;
    struct rt_vol_internal vol;	/* scaled version */
    union record *rec;
    struct bu_vls str = BU_VLS_INIT_ZERO;

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_VOL) return -1;
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);
    vol = *vip;			/* struct copy */

    /* Apply scale factor */
    vol.mat[15] /= local2mm;

    BU_CK_EXTERNAL(ep);
    ep->ext_nbytes = sizeof(union record)*DB_SS_NGRAN;
    ep->ext_buf = (uint8_t *)bu_calloc(1, ep->ext_nbytes, "vol external");
    rec = (union record *)ep->ext_buf;

    bu_vls_struct_print(&str, rt_vol_parse, (char *)&vol);

    rec->ss.ss_id = DBID_STRSOL;
    bu_strlcpy(rec->ss.ss_keyword, "vol", sizeof(rec->ss.ss_keyword));
    bu_strlcpy(rec->ss.ss_args, bu_vls_addr(&str), DB_SS_LEN);
    bu_vls_free(&str);

    return 0;
}


static size_t
vol_from_file(const char *file, size_t xdim, size_t ydim, size_t zdim, unsigned char **map)
{
    size_t y;
    size_t z;
    size_t ret = 0;
    size_t nbytes;
    FILE *fp;

    /* Get bit map from .bw(5) file */
    nbytes = (xdim+VOL_XWIDEN*2)*
	(ydim+VOL_YWIDEN*2)*
	(zdim+VOL_ZWIDEN*2);
    *map = (unsigned char *)bu_calloc(1, nbytes, "vol_import4 bitmap");

    bu_semaphore_acquire(BU_SEM_SYSCALL);		/* lock */
    if ((fp = fopen(file, "rb")) == NULL) {
	perror(file);
	bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */
	return 0;
    }
    bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */

    /* Because of in-memory padding, read each scanline separately */
    for (z = 0; z < zdim; z++) {
	for (y = 0; y < ydim; y++) {
	    size_t fret;
	    void *data = &VOLMAP(*map, xdim, ydim, 0, y, z);

	    bu_semaphore_acquire(BU_SEM_SYSCALL);	/* lock */
	    fret = fread(data, xdim, 1, fp);		/* res_syscall */
	    bu_semaphore_release(BU_SEM_SYSCALL);	/* unlock */
	    if (fret < 1) {
		bu_log("rt_vol_import4(%s): Unable to read whole VOL, y=%zu, z=%zu\n", file, y, z);
		bu_semaphore_acquire(BU_SEM_SYSCALL);	/* lock */
		fclose(fp);
		bu_semaphore_release(BU_SEM_SYSCALL);	/* unlock */
		return 0;
	    }
	    ret += xdim;
	}
    }
    bu_semaphore_acquire(BU_SEM_SYSCALL);		/* lock */
    fclose(fp);
    bu_semaphore_release(BU_SEM_SYSCALL);		/* unlock */

    return ret;
}


/**
 * Read VOL data from external file
 * Returns :
 * 0 success
 * !0 fail
 */
static int
vol_file_data(struct rt_vol_internal *vip)
{
   size_t nbytes;

   size_t bytes = vip->xdim * vip->ydim * vip->zdim;
	nbytes = vol_from_file(vip->name, vip->xdim, vip->ydim, vip->zdim, &vip->map);
	if (nbytes != bytes) {
	    bu_log("WARNING: unexpected VOL bytes (read %zu, expected %zu) in %s\n", nbytes, bytes, vip->name);
	}

   return 0;
}


/**
 * Read VOL data from in database object
 * Returns :
 * 0 success
 * !0 fail
 */
static int
get_obj_data(struct rt_vol_internal *vip, const struct db_i *dbip)
{
    struct rt_binunif_internal *bip;
    int ret;
    int nbytes;

    if (!vip || !dbip)
	return -1;

    BU_ALLOC(vip->bip, struct rt_db_internal);

    ret = rt_retrieve_binunif(vip->bip, dbip, vip->name);
    if (ret)
	return -1;

    if (RT_G_DEBUG & RT_DEBUG_HF) {
	bu_log("db_internal magic: 0x%08x  major: %d  minor: %d\n",
		vip->bip->idb_magic,
		vip->bip->idb_major_type,
		vip->bip->idb_minor_type);
    }

    bip = (struct rt_binunif_internal *)vip->bip->idb_ptr;

    if (RT_G_DEBUG & RT_DEBUG_HF)
	bu_log("binunif magic: 0x%08x  type: %d count:%zu data[0]:%u\n",
		bip->magic, bip->type, bip->count, bip->u.uint8[0]);

    if (bip->type != DB5_MINORTYPE_BINU_8BITINT_U
	|| (size_t)bip->count != (size_t)(vip->xdim*vip->ydim*vip->zdim))
    {
	size_t i = 0;
	size_t size;
	struct bu_vls binudesc = BU_VLS_INIT_ZERO;
	rt_binunif_describe(&binudesc, vip->bip, 0, dbip->dbi_base2local);

	/* skip the first title line*/
	size = bu_vls_strlen(&binudesc);
	while (size > 0 && i < size && bu_vls_cstr(&binudesc)[0] != '\n') {
	    bu_vls_nibble(&binudesc, 1);
	}
	if (bu_vls_cstr(&binudesc)[0] == '\n')
	    bu_vls_nibble(&binudesc, 1);

	bu_log("ERROR: Binary object '%s' has invalid data (expected type %d, found %d).\n"
	       "       Expecting %zu 8-bit unsigned char (nuc) integer data values.\n"
	       "       Encountered %s\n",
	       vip->name,
	       DB5_MINORTYPE_BINU_8BITINT_U,
	       bip->type,
	       (size_t)(vip->xdim*vip->ydim*vip->zdim),
	       bu_vls_cstr(&binudesc));
	return -2;
    }

    nbytes = (vip->xdim+VOL_XWIDEN*2)*(vip->ydim+VOL_YWIDEN*2)*(vip->zdim+VOL_ZWIDEN*2);

    size_t y, z;
    if (!vip->map) {
	unsigned char* cp;

  vip->map = (unsigned char *)bu_calloc(1, nbytes, "vol_import4 bitmap");
	cp = (unsigned char *)bip->u.uint8;

  for (z = 0; z < vip->zdim; z++) {
	   for (y = 0; y < vip->ydim; y++) {
       void *data = &VOLMAP(vip->map, vip->xdim, vip->ydim, 0, y, z);

	     memcpy(data, cp, vip->xdim);
       cp+= vip->xdim;
	   }
  }
    }
    return 0;
}

/**
 * Retrieve VOL data from data source
 * Returns :
 * 0 success
 * !0 fail
 */
static int
get_vol_data(struct rt_vol_internal *vip, const struct db_i *dbip)
{
    switch (vip->datasrc) {
	case RT_VOL_SRC_FILE:
	    /* Retrieve the data from an external file */
	    if (RT_G_DEBUG & RT_DEBUG_HF)
		bu_log("getting data from file \"%s\"\n", vip->name);

	    if(vol_file_data(vip) != 0) {
		return 1;
	    }
	    else {
		return 0;
	    }
	    break;
	case RT_VOL_SRC_OBJ:
	    /* Retrieve the data from an internal db object */
	    if (RT_G_DEBUG & RT_DEBUG_HF)
		bu_log("getting data from object \"%s\"\n", vip->name);

	    if (get_obj_data(vip, dbip) != 0) {
		return 1;
	    } else {
		RT_CK_DB_INTERNAL(vip->bip);
		RT_CK_BINUNIF(vip->bip->idb_ptr);
		return 0;
	    }
	    break;
	default:
	    bu_log("%s:%d Odd vol data src '%c' s/b '%c' or '%c'\n",
		    __FILE__, __LINE__, vip->datasrc,
		    RT_VOL_SRC_FILE, RT_VOL_SRC_OBJ);
    }

    if (dbip)
	bu_log("%s", dbip->dbi_filename);
    return 0; //temporary
}

int
rt_vol_mat(struct rt_db_internal *rop, const mat_t mat, const struct rt_db_internal *ip)
{
    if (!rop || !ip || !mat)
	return BRLCAD_OK;

    struct rt_vol_internal *tip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_EBM_CK_MAGIC(tip);
    struct rt_vol_internal *top = (struct rt_vol_internal *)rop->idb_ptr;
    RT_EBM_CK_MAGIC(top);

    /* Apply transform */
    mat_t tmp;
    bn_mat_mul(tmp, mat, tip->mat);
    MAT_COPY(top->mat, tmp);

    return BRLCAD_OK;
}

/**
 * Read in the information from the string solid record.
 * Then, as a service to the application, read in the bitmap
 * and set up some of the associated internal variables.
 */
int
rt_vol_import5(struct rt_db_internal *ip, const struct bu_external *ep, const fastf_t *mat, const struct db_i *dbip)
{
    register struct rt_vol_internal *vip;
    struct bu_vls str = BU_VLS_INIT_ZERO;

    if (dbip) RT_CK_DBI(dbip);

    BU_CK_EXTERNAL(ep);

    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_VOL;
    ip->idb_meth = &OBJ[ID_VOL];
    BU_ALLOC(ip->idb_ptr, struct rt_vol_internal);

    vip = (struct rt_vol_internal *)ip->idb_ptr;
    vip->magic = RT_VOL_INTERNAL_MAGIC;

    /* Establish defaults */
    MAT_IDN(vip->mat);
    vip->lo = 0;
    vip->hi = 255;

    /* default source is file for compatibility with old VOL */
    vip->datasrc = RT_VOL_SRC_FILE;

    /* Default VOL cell size in ideal coordinates is one unit/cell */
    VSETALL(vip->cellsize, 1);

    bu_vls_strncpy(&str, (const char *)ep->ext_buf, ep->ext_nbytes);
    if (bu_struct_parse(&str, rt_vol_parse, (char *)vip, NULL) < 0) {
	bu_vls_free(&str);
	return -2;
    }
    bu_vls_free(&str);
    /* Check for reasonable values */
    if (vip->name[0] == '\0'
	|| vip->xdim < 1 || vip->ydim < 1 || vip->zdim < 1
	|| vip->mat[15] <= 0.0 || vip->hi > 255)
    {
	bu_struct_print("Unreasonable VOL parameters", rt_vol_parse, (char *)vip);
	return -1;
    }

    if (!mat)
	mat = bn_mat_identity;

    /* Apply any modeling transforms to get final matrix */
    rt_vol_mat(ip, mat, ip);

    if (get_vol_data(vip, dbip) == 1)
	bu_log("Couldn't find the associated file/object %s",vip->name);

    return 0;
}


/**
 * The name will be added by the caller.
 */
int
rt_vol_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_vol_internal *vip;
    struct rt_vol_internal vol;	/* scaled version */
    struct bu_vls str = BU_VLS_INIT_ZERO;

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_VOL) return -1;
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);
    vol = *vip;			/* struct copy */

    /* Apply scale factor */
    vol.mat[15] /= local2mm;

    BU_CK_EXTERNAL(ep);

    bu_vls_struct_print(&str, rt_vol_parse, (char *)&vol);
    ep->ext_nbytes = bu_vls_strlen(&str);
    ep->ext_buf = (uint8_t *)bu_calloc(1, ep->ext_nbytes, "vol external");

    bu_strlcpy((char *)ep->ext_buf, bu_vls_addr(&str), ep->ext_nbytes);
    bu_vls_free(&str);

    return 0;
}


/**
 * Make human-readable formatted presentation of this solid.
 * First line describes type of solid.
 * Additional lines are indented one tab, and give parameter values.
 */
int
rt_vol_describe(struct bu_vls *str, const struct rt_db_internal *ip, int UNUSED(verbose), double mm2local)
{
    struct rt_vol_internal *vip = (struct rt_vol_internal *)ip->idb_ptr;
    register int i;
    struct bu_vls substr = BU_VLS_INIT_ZERO;
    vect_t local;

    RT_VOL_CK_MAGIC(vip);

    bu_vls_strcat(str, "thresholded volumetric solid (VOL)\n");

    /* pretty-print dimensions in local, not storage (mm) units */
    VSCALE(local, vip->cellsize, mm2local);
    if (vip->datasrc == RT_VOL_SRC_FILE) {
    bu_vls_printf(&substr, "\tfile=\"%s\"\n\tw=%u n=%u d=%u\n\tlo=%u hi=%u\n\tsize=%g,%g,%g\n",
		  vip->name,
		  vip->xdim, vip->ydim, vip->zdim, vip->lo, vip->hi,
		  V3INTCLAMPARGS(local));
    } else {
      bu_vls_printf(&substr, "\tobject name=\"%s\"\n\tw=%u n=%u d=%u\n\tlo=%u hi=%u\n\tsize=%g,%g,%g\n",
		  vip->name,
		  vip->xdim, vip->ydim, vip->zdim, vip->lo, vip->hi,
		  V3INTCLAMPARGS(local));
    }

    bu_vls_vlscat(str, &substr);

    bu_vls_strcat(str, "\tmat=");
    for (i = 0; i < 15; i++) {
	bu_vls_trunc(&substr, 0);
	bu_vls_printf(&substr, "%g, ", INTCLAMP(vip->mat[i]));
	bu_vls_vlscat(str, &substr);
    }
    bu_vls_trunc(&substr, 0);
    bu_vls_printf(&substr, "%g\n", INTCLAMP(vip->mat[i]));
    bu_vls_vlscat(str, &substr);

    bu_vls_free(&substr);

    return 0;
}


/**
 * Free the storage associated with the rt_db_internal version of this solid.
 */
void
rt_vol_ifree(struct rt_db_internal *ip)
{
    register struct rt_vol_internal *vip;

    RT_CK_DB_INTERNAL(ip);

    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    /* should be stolen by vol_specific, but check just in case */
    if (vip->map)
	bu_free((char *)vip->map, "vol bitmap");

    vip->magic = 0;			/* sanity */
    vip->map = (unsigned char *)0;
    bu_free((char *)vip, "vol ifree");
    ip->idb_ptr = ((void *)0);	/* sanity */
}


/**
 * Calculate bounding RPP for vol
 */
int
rt_vol_bbox(struct rt_db_internal *ip, point_t *min, point_t *max, const struct bn_tol *UNUSED(tol))
{
    register struct rt_vol_internal *vip;
    vect_t v1, localspace;

    RT_CK_DB_INTERNAL(ip);
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    /* Find bounding RPP of rotated local RPP */
    VSETALL(v1, 0);
    VSET(localspace, vip->xdim*vip->cellsize[0], vip->ydim*vip->cellsize[1], vip->zdim*vip->cellsize[2]);/* type conversion */
    bg_rotate_bbox((*min), (*max), vip->mat, v1, localspace);
    return 0;
}


/**
 * Returns -
 * 0 OK
 * !0 Failure
 *
 * Implicit return -
 * A struct rt_vol_specific is created, and its address is stored
 * in stp->st_specific for use by rt_vol_shot().
 */
int
rt_vol_prep(struct soltab *stp, struct rt_db_internal *ip, struct rt_i *rtip)
{
    struct rt_vol_internal *vip;
    register struct rt_vol_specific *volp;
    vect_t norm;
    vect_t radvec;
    vect_t diam;

    RT_CK_SOLTAB(stp);
    RT_CK_DB_INTERNAL(ip);
    if (rtip) RT_CK_RTI(rtip);

    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    BU_GET(volp, struct rt_vol_specific);
    volp->vol_i = *vip;		/* struct copy */
    vip->map = (unsigned char *)0;	/* "steal" the bitmap storage */

    /* build Xform matrix from model(world) to ideal(local) space */
    bn_mat_inv(volp->vol_mat, vip->mat);

    /* Pre-compute the necessary normals.  Rotate only. */
    VSET(norm, 1, 0, 0);
    MAT3X3VEC(volp->vol_xnorm, vip->mat, norm);
    VSET(norm, 0, 1, 0);
    MAT3X3VEC(volp->vol_ynorm, vip->mat, norm);
    VSET(norm, 0, 0, 1);
    MAT3X3VEC(volp->vol_znorm, vip->mat, norm);

    stp->st_specific = (void *)volp;

    /* Find bounding RPP of rotated local RPP */
    if (rt_vol_bbox(ip, &(stp->st_min), &(stp->st_max), &rtip->rti_tol)) return 1;
    VSET(volp->vol_large,
	 volp->vol_i.xdim*vip->cellsize[0], volp->vol_i.ydim*vip->cellsize[1], volp->vol_i.zdim*vip->cellsize[2]);/* type conversion */

    /* for now, VOL origin in ideal coordinates is at origin */
    VSETALL(volp->vol_origin, 0);
    VADD2(volp->vol_large, volp->vol_large, volp->vol_origin);

    VSUB2(diam, stp->st_max, stp->st_min);
    VADD2SCALE(stp->st_center, stp->st_min, stp->st_max, 0.5);
    VSCALE(radvec, diam, 0.5);
    stp->st_aradius = stp->st_bradius = MAGNITUDE(radvec);

    return 0;		/* OK */
}


void
rt_vol_print(register const struct soltab *stp)
{
    register const struct rt_vol_specific *volp =
	(struct rt_vol_specific *)stp->st_specific;

    bu_log("vol file = %s\n", volp->vol_i.name);
    bu_log("dimensions = (%u, %u, %u)\n",
	   volp->vol_i.xdim, volp->vol_i.ydim,
	   volp->vol_i.zdim);
    VPRINT("model cellsize", volp->vol_i.cellsize);
    VPRINT("model grid origin", volp->vol_origin);
}


/**
 * Given one ray distance, return the normal and
 * entry/exit point.
 * This is mostly a matter of translating the stored
 * code into the proper normal.
 */
void
rt_vol_norm(register struct hit *hitp, struct soltab *stp, register struct xray *rp)
{
    register struct rt_vol_specific *volp =
	(struct rt_vol_specific *)stp->st_specific;

    VJOIN1(hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir);

    switch (hitp->hit_surfno) {
	case NORM_XPOS:
	    VMOVE(hitp->hit_normal, volp->vol_xnorm);
	    break;
	case NORM_XNEG:
	    VREVERSE(hitp->hit_normal, volp->vol_xnorm);
	    break;

	case NORM_YPOS:
	    VMOVE(hitp->hit_normal, volp->vol_ynorm);
	    break;
	case NORM_YNEG:
	    VREVERSE(hitp->hit_normal, volp->vol_ynorm);
	    break;

	case NORM_ZPOS:
	    VMOVE(hitp->hit_normal, volp->vol_znorm);
	    break;
	case NORM_ZNEG:
	    VREVERSE(hitp->hit_normal, volp->vol_znorm);
	    break;

	default:
	    bu_log("rt_vol_norm(%s): surfno=%d bad\n",
		   stp->st_name, hitp->hit_surfno);
	    VSETALL(hitp->hit_normal, 0);
	    break;
    }
}


/**
 * Everything has sharp edges.  This makes things easy.
 */
void
rt_vol_curve(register struct curvature *cvp, register struct hit *hitp, struct soltab *stp)
{
    if (!cvp || !hitp)
	return;
    RT_CK_HIT(hitp);
    if (stp) RT_CK_SOLTAB(stp);

    bn_vec_ortho(cvp->crv_pdir, hitp->hit_normal);
    cvp->crv_c1 = cvp->crv_c2 = 0;
}


/**
 * Map the hit point in 2-D into the range 0..1
 * untransformed X becomes U, and Y becomes V.
 */
void
rt_vol_uv(struct application *ap, struct soltab *stp, register struct hit *hitp, register struct uvcoord *uvp)
{
    if (ap) RT_CK_APPLICATION(ap);
    if (stp) RT_CK_SOLTAB(stp);
    if (hitp) RT_CK_SOLTAB(stp);
    if (!uvp)
	return;

    /* XXX uv should be xy in ideal space */
}


void
rt_vol_free(struct soltab *stp)
{
    register struct rt_vol_specific *volp =
	(struct rt_vol_specific *)stp->st_specific;

    /* specific steals map from vip, release here */
    if (volp->vol_i.map) {
	bu_free((char *)volp->vol_i.map, "vol_map");
	volp->vol_i.map = NULL; /* sanity */
    }
    BU_PUT(volp, struct rt_vol_specific);
}


int
rt_vol_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct bg_tess_tol *UNUSED(ttol), const struct bn_tol *UNUSED(tol), const struct bview *UNUSED(info))
{
    register struct rt_vol_internal *vip;
    int x, y, z;
    register short v1, v2;
    point_t a, b, c, d;

    BU_CK_LIST_HEAD(vhead);
    RT_CK_DB_INTERNAL(ip);
    struct bu_list *vlfree = &rt_vlfree;
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    if (!vip->map) {
	return 1;
    }

    /*
     * Scan across in Z & X.  For each X position, scan down Y,
     * looking for the longest run of edge.
     */
    for (z=-1; z<=(int)vip->zdim; z++) {
	for (x=-1; x<=(int)vip->xdim; x++) {
	    for (y=-1; y<=(int)vip->ydim; y++) {
		v1 = VOL(vip, x, y, z);
		v2 = VOL(vip, x+1, y, z);
		if (OK(vip, (size_t)v1) == OK(vip, (size_t)v2)) continue;
		/* Note start point, continue scan */
		VSET(a, x+0.5, y-0.5, z-0.5);
		VSET(b, x+0.5, y-0.5, z+0.5);
		for (++y; y<=(int)vip->ydim; y++) {
		    v1 = VOL(vip, x, y, z);
		    v2 = VOL(vip, x+1, y, z);
		    if (OK(vip, (size_t)v1) == OK(vip, (size_t)v2))
			break;
		}
		/* End of run of edge.  One cell beyond. */
		VSET(c, x+0.5, y-0.5, z+0.5);
		VSET(d, x+0.5, y-0.5, z-0.5);
		rt_vol_plate(a, b, c, d, vip->mat, vlfree, vhead, vip);
	    }
	}
    }

    /*
     * Scan up in Z & Y.  For each Y position, scan across X
     */
    for (z=-1; z<=(int)vip->zdim; z++) {
	for (y=-1; y<=(int)vip->ydim; y++) {
	    for (x=-1; x<=(int)vip->xdim; x++) {
		v1 = VOL(vip, x, y, z);
		v2 = VOL(vip, x, y+1, z);
		if (OK(vip, (size_t)v1) == OK(vip, (size_t)v2)) continue;
		/* Note start point, continue scan */
		VSET(a, x-0.5, y+0.5, z-0.5);
		VSET(b, x-0.5, y+0.5, z+0.5);
		for (++x; x<=(int)vip->xdim; x++) {
		    v1 = VOL(vip, x, y, z);
		    v2 = VOL(vip, x, y+1, z);
		    if (OK(vip, (size_t)v1) == OK(vip, (size_t)v2))
			break;
		}
		/* End of run of edge.  One cell beyond */
		VSET(c, (x-0.5), (y+0.5), (z+0.5));
		VSET(d, (x-0.5), (y+0.5), (z-0.5));
		rt_vol_plate(a, b, c, d, vip->mat, vlfree, vhead, vip);
	    }
	}
    }

    /*
     * Scan across in Y & X.  For each X position pair edge, scan up Z.
     */
    for (x=-1; x<=(int)vip->xdim; x++) {
	for (z=-1; z<=(int)vip->zdim; z++) {
	    for (y=-1; y<=(int)vip->ydim; y++) {
		v1 = VOL(vip, x, y, z);
		v2 = VOL(vip, x, y, z+1);
		if (OK(vip, (size_t)v1) == OK(vip, (size_t)v2)) continue;
		/* Note start point, continue scan */
		VSET(a, (x-0.5), (y-0.5), (z+0.5));
		VSET(b, (x+0.5), (y-0.5), (z+0.5));
		for (++y; y<=(int)vip->ydim; y++) {
		    v1 = VOL(vip, x, y, z);
		    v2 = VOL(vip, x, y, z+1);
		    if (OK(vip, (size_t)v1) == OK(vip, (size_t)v2))
			break;
		}
		/* End of run of edge.  One cell beyond */
		VSET(c, (x+0.5), (y-0.5), (z+0.5));
		VSET(d, (x-0.5), (y-0.5), (z+0.5));
		rt_vol_plate(a, b, c, d, vip->mat, vlfree, vhead, vip);
	    }
	}
    }
    return 0;
}


void
rt_vol_plate(point_t a, point_t b, point_t c, point_t d, register mat_t mat, struct bu_list *vlfree, register struct bu_list *vhead, register struct rt_vol_internal *vip)
{
    point_t s;		/* scaled original point */
    point_t arot, prot;

    BU_CK_LIST_HEAD(vhead);

    VELMUL(s, vip->cellsize, a);
    MAT4X3PNT(arot, mat, s);
    BV_ADD_VLIST(vlfree, vhead, arot, BV_VLIST_LINE_MOVE);

    VELMUL(s, vip->cellsize, b);
    MAT4X3PNT(prot, mat, s);
    BV_ADD_VLIST(vlfree, vhead, prot, BV_VLIST_LINE_DRAW);

    VELMUL(s, vip->cellsize, c);
    MAT4X3PNT(prot, mat, s);
    BV_ADD_VLIST(vlfree, vhead, prot, BV_VLIST_LINE_DRAW);

    VELMUL(s, vip->cellsize, d);
    MAT4X3PNT(prot, mat, s);
    BV_ADD_VLIST(vlfree, vhead, prot, BV_VLIST_LINE_DRAW);

    BV_ADD_VLIST(vlfree, vhead, arot, BV_VLIST_LINE_DRAW);
}


int
rt_vol_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct bg_tess_tol *ttol, const struct bn_tol *tol)
{
    struct rt_vol_internal *vip;
    register size_t x, y, z;
    int i;
    struct shell *s;
    struct vertex *verts[4];
    struct faceuse *fu;
    struct model *m_tmp;
    struct nmgregion *r_tmp;
    struct bu_list *vlfree = &rt_vlfree;

    NMG_CK_MODEL(m);
    BN_CK_TOL(tol);
    BG_CK_TESS_TOL(ttol);

    RT_CK_DB_INTERNAL(ip);
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    /* make region, shell, vertex */
    m_tmp = nmg_mm();
    r_tmp = nmg_mrsv(m_tmp);
    s = BU_LIST_FIRST(shell, &r_tmp->s_hd);

    for (x = 0; x < vip->xdim; x++) {
	for (y = 0; y < vip->ydim; y++) {
	    for (z = 0; z < vip->zdim; z++) {
		point_t pt, pt1;

		/* skip empty cells */
		if (!OK(vip, VOL(vip, x, y, z)))
		    continue;

		/* check neighboring cells, make a face where needed */

		/* check z+1 */
		if (!OK(vip, VOL(vip, x, y, z+1))) {
		    for (i = 0; i < 4; i++)
			verts[i] = (struct vertex *)NULL;

		    fu = nmg_cface(s, verts, 4);

		    VSET(pt, x+.5, y-.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[0], pt);
		    VSET(pt, x+.5, y+.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[1], pt);
		    VSET(pt, x-.5, y+.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[2], pt);
		    VSET(pt, x-.5, y-.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[3], pt);

		    if (nmg_fu_planeeqn(fu, tol))
			goto fail;
		}

		/* check z-1 */
		if (!OK(vip, VOL(vip, x, y, z-1))) {
		    for (i = 0; i < 4; i++)
			verts[i] = (struct vertex *)NULL;

		    fu = nmg_cface(s, verts, 4);

		    VSET(pt, x+.5, y-.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[3], pt);
		    VSET(pt, x+.5, y+.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[2], pt);
		    VSET(pt, x-.5, y+.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[1], pt);
		    VSET(pt, x-.5, y-.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[0], pt);

		    if (nmg_fu_planeeqn(fu, tol))
			goto fail;
		}

		/* check y+1 */
		if (!OK(vip, VOL(vip, x, y+1, z))) {
		    for (i = 0; i < 4; i++)
			verts[i] = (struct vertex *)NULL;

		    fu = nmg_cface(s, verts, 4);

		    VSET(pt, x+.5, y+.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[0], pt);
		    VSET(pt, x+.5, y+.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[1], pt);
		    VSET(pt, x-.5, y+.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[2], pt);
		    VSET(pt, x-.5, y+.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[3], pt);

		    if (nmg_fu_planeeqn(fu, tol))
			goto fail;
		}

		/* check y-1 */
		if (!OK(vip, VOL(vip, x, y-1, z))) {
		    for (i = 0; i < 4; i++)
			verts[i] = (struct vertex *)NULL;

		    fu = nmg_cface(s, verts, 4);

		    VSET(pt, x+.5, y-.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[3], pt);
		    VSET(pt, x+.5, y-.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[2], pt);
		    VSET(pt, x-.5, y-.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[1], pt);
		    VSET(pt, x-.5, y-.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[0], pt);

		    if (nmg_fu_planeeqn(fu, tol))
			goto fail;
		}

		/* check x+1 */
		if (!OK(vip, VOL(vip, x+1, y, z))) {
		    for (i = 0; i < 4; i++)
			verts[i] = (struct vertex *)NULL;

		    fu = nmg_cface(s, verts, 4);

		    VSET(pt, x+.5, y-.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[0], pt);
		    VSET(pt, x+.5, y+.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[1], pt);
		    VSET(pt, x+.5, y+.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[2], pt);
		    VSET(pt, x+.5, y-.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[3], pt);

		    if (nmg_fu_planeeqn(fu, tol))
			goto fail;
		}

		/* check x-1 */
		if (!OK(vip, VOL(vip, x-1, y, z))) {
		    for (i = 0; i < 4; i++)
			verts[i] = (struct vertex *)NULL;

		    fu = nmg_cface(s, verts, 4);

		    VSET(pt, x-.5, y-.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[3], pt);
		    VSET(pt, x-.5, y+.5, z-.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[2], pt);
		    VSET(pt, x-.5, y+.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[1], pt);
		    VSET(pt, x-.5, y-.5, z+.5);
		    VELMUL(pt1, vip->cellsize, pt);
		    MAT4X3PNT(pt, vip->mat, pt1);
		    nmg_vertex_gv(verts[0], pt);

		    if (nmg_fu_planeeqn(fu, tol))
			goto fail;
		}
	    }
	}
    }

    nmg_region_a(r_tmp, tol);

    /* fuse model */
    nmg_model_fuse(m_tmp, vlfree, tol);

    /* simplify shell */
    nmg_shell_coplanar_face_merge(s, tol, 1, vlfree);

    /* kill snakes */
    for (BU_LIST_FOR(fu, faceuse, &s->fu_hd)) {
	struct loopuse *lu;

	NMG_CK_FACEUSE(fu);

	if (fu->orientation != OT_SAME)
	    continue;

	for (BU_LIST_FOR(lu, loopuse, &fu->lu_hd))
	    (void)nmg_kill_snakes(lu, vlfree);
    }

    (void)nmg_unbreak_region_edges((uint32_t *)(&s->l), vlfree);

    (void)nmg_mark_edges_real((uint32_t *)&s->l, vlfree);

    nmg_merge_models(m, m_tmp);
    *r = r_tmp;

    return 0;

fail:
    nmg_km(m_tmp);
    *r = (struct nmgregion *)NULL;

    return -1;
}


int
rt_vol_params(struct pc_pc_set *UNUSED(ps), const struct rt_db_internal *ip)
{
    if (ip) RT_CK_DB_INTERNAL(ip);

    return 0;			/* OK */
}


void
rt_vol_centroid(point_t *cent, const struct rt_db_internal *ip)
{
    register struct rt_vol_internal *vip;
    size_t x, y, z;
    size_t cnt;
    fastf_t x_tot, y_tot, z_tot;
    point_t p;

    RT_CK_DB_INTERNAL(ip);
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    cnt = 0;
    x_tot = y_tot = z_tot = 0.0;

    for (x = 0; x < vip->xdim; x++) {
	for (y = 0; y < vip->ydim; y++) {
	    for (z = 0; z < vip->zdim; z++) {
		if (OK(vip, VOL(vip, x, y, z))) {
		    x_tot += (x+.5) * (vip->cellsize[X]);
		    y_tot += (y+.5) * (vip->cellsize[Y]);
		    z_tot += (z+.5) * (vip->cellsize[Z]);
		    cnt++;
		}
	    }
	}
    }

    p[X]=x_tot/cnt;
    p[Y]=y_tot/cnt;
    p[Z]=z_tot/cnt;

    MAT4X3PNT(*cent, vip->mat, p);
}


/*
 * Computes the surface area of a volume by transforming each of the vertices by
 * the matrix and then summing the area of the faces of each cell necessary.
 * The vertices are numbered from left to right, front to back, bottom to top.
 */
void
rt_vol_surf_area(fastf_t *area, const struct rt_db_internal *ip)
{
    struct rt_vol_internal *vip;
    unsigned int x, y, z;
    point_t x0, x1, x2, x3, x4, x5, x6, x7;
    point_t _x0, _x1, _x2, _x3, _x4, _x5, _x6, _x7;
    vect_t d3_0, d2_1, d7_4, d6_5, d6_0, d4_2, d4_1, d5_0, d5_3, d7_1, d7_2, d6_3, _cross;
    fastf_t _x, _y, _z, det, _area = 0.0;

    if (area == NULL || ip == NULL) return;
    RT_CK_DB_INTERNAL(ip);
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    det = fabs(bn_mat_determinant(vip->mat));
    if (EQUAL(det, 0.0)) {
	*area = -1.0;
	return;
    }

    for (x = 0; x < vip->xdim; x++) {
	for (y = 0; y < vip->ydim; y++) {
	    for (z = 0; z < vip->zdim; z++) {
		if (OK(vip, VOL(vip, x, y, z))) {
		    _x = (fastf_t)x;
		    _y = (fastf_t)y;
		    _z = (fastf_t)z;

		    VSET(_x0, _x - 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x1, _x + 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x2, _x - 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x3, _x + 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x4, _x - 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);
		    VSET(_x5, _x + 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);
		    VSET(_x6, _x - 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);
		    VSET(_x7, _x + 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);

		    MAT4X3PNT(x0, vip->mat, _x0);
		    MAT4X3PNT(x1, vip->mat, _x1);
		    MAT4X3PNT(x2, vip->mat, _x2);
		    MAT4X3PNT(x3, vip->mat, _x3);
		    MAT4X3PNT(x4, vip->mat, _x4);
		    MAT4X3PNT(x5, vip->mat, _x5);
		    MAT4X3PNT(x6, vip->mat, _x6);
		    MAT4X3PNT(x7, vip->mat, _x7);

		    if (!OK(vip, VOL(vip, x + 1, y, z))) {
			VSUB2(d5_3, x5, x3);
			VSUB2(d7_1, x7, x1);
			VCROSS(_cross, d5_3, d7_1);
			_area += 0.5 * MAGNITUDE(_cross);
		    }
		    if (!OK(vip, VOL(vip, x - 1, y, z))) {
			VSUB2(d6_0, x6, x0);
			VSUB2(d4_2, x4, x2);
			VCROSS(_cross, d6_0, d4_2);
			_area += 0.5 * MAGNITUDE(_cross);
		    }

		    if (!OK(vip, VOL(vip, x, y + 1, z))) {
			VSUB2(d7_2, x7, x2);
			VSUB2(d6_3, x6, x3);
			VCROSS(_cross, d7_2, d6_3);
			_area += 0.5 * MAGNITUDE(_cross);
		    }
		    if (!OK(vip, VOL(vip, x, y - 1, z))) {
			VSUB2(d4_1, x4, x1);
			VSUB2(d5_0, x5, x0);
			VCROSS(_cross, d4_1, d5_0);
			_area += 0.5 * MAGNITUDE(_cross);
		    }

		    if (!OK(vip, VOL(vip, x, y, z + 1))) {
			VSUB2(d3_0, x3, x0);
			VSUB2(d2_1, x2, x1);
			VCROSS(_cross, d3_0, d2_1);
			_area += 0.5 * MAGNITUDE(_cross);
		    }
		    if (!OK(vip, VOL(vip, x, y, z - 1))) {
			VSUB2(d7_4, x7, x4);
			VSUB2(d6_5, x6, x5);
			VCROSS(_cross, d7_4, d6_5);
			_area += 0.5 * MAGNITUDE(_cross);
		    }
		}
	    }
	}
    }
    *area = _area;
}


/*
 * A paper at http://www.osti.gov/scitech/servlets/purl/632793 gives a method
 * for calculating the volume of hexahedrens from the eight vertices.
 *
 * The eight vertices are calculated, then transformed by the matrix and the
 * volume calculated from that.
 */
void
rt_vol_volume(fastf_t *volume, const struct rt_db_internal *ip)
{
    struct rt_vol_internal *vip;
    unsigned int x, y, z;
    point_t x0, x1, x2, x3, x4, x5, x6, x7;
    point_t _x0, _x1, _x2, _x3, _x4, _x5, _x6, _x7;
    vect_t d7_0, d1_0, d3_5, d4_0, d5_6, d2_0, d6_3, cross1, cross2, cross3;
    fastf_t _x, _y, _z, det, _vol = 0.0;

    if (volume == NULL || ip == NULL) return;
    RT_CK_DB_INTERNAL(ip);
    vip = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vip);

    det = fabs(bn_mat_determinant(vip->mat));
    if (EQUAL(det, 0.0)) {
	*volume = -1.0;
	return;
    }

    for (x = 0; x < vip->xdim; x++) {
	for (y = 0; y < vip->ydim; y++) {
	    for (z = 0; z < vip->zdim; z++) {
		if (OK(vip, VOL(vip, x, y, z))) {
		    _x = (fastf_t)x;
		    _y = (fastf_t)y;
		    _z = (fastf_t)z;

		    VSET(_x0, _x - 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x1, _x + 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x2, _x - 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x3, _x + 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z - 0.5 * vip->cellsize[Z]);
		    VSET(_x4, _x - 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);
		    VSET(_x5, _x + 0.5 * vip->cellsize[X], _y - 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);
		    VSET(_x6, _x - 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);
		    VSET(_x7, _x + 0.5 * vip->cellsize[X], _y + 0.5 * vip->cellsize[Y], _z + 0.5 * vip->cellsize[Z]);

		    MAT4X3PNT(x0, vip->mat, _x0);
		    MAT4X3PNT(x1, vip->mat, _x1);
		    MAT4X3PNT(x2, vip->mat, _x2);
		    MAT4X3PNT(x3, vip->mat, _x3);
		    MAT4X3PNT(x4, vip->mat, _x4);
		    MAT4X3PNT(x5, vip->mat, _x5);
		    MAT4X3PNT(x6, vip->mat, _x6);
		    MAT4X3PNT(x7, vip->mat, _x7);

		    VSUB2(d7_0, x7, x0);
		    VSUB2(d1_0, x1, x0);
		    VSUB2(d3_5, x3, x5);
		    VCROSS(cross1, d1_0, d3_5);

		    VSUB2(d4_0, x4, x0);
		    VSUB2(d5_6, x5, x6);
		    VCROSS(cross2, d4_0, d5_6);

		    VSUB2(d2_0, x2, x0);
		    VSUB2(d6_3, x6, x3);
		    VCROSS(cross3, d2_0, d6_3);

		    _vol += (1.0/6.0) * (VDOT(d7_0, cross1) + VDOT(d7_0, cross2) + VDOT(d7_0, cross3));
		}
	    }
	}
    }
    *volume = fabs(_vol);
}

const char *
rt_vol_keypoint(point_t *pt, const char *keystr, const mat_t mat, const struct rt_db_internal *ip, const struct bn_tol *UNUSED(tol))
{
    if (!pt || !ip)
	return NULL;

    point_t mpt = VINIT_ZERO;
    struct rt_vol_internal *vol = (struct rt_vol_internal *)ip->idb_ptr;
    RT_VOL_CK_MAGIC(vol);

    static const char *default_keystr = "V";
    const char *k = (keystr) ? keystr : default_keystr;

    if (BU_STR_EQUAL(k, default_keystr)) {
	point_t pnt = VINIT_ZERO;
	MAT4X3PNT(mpt, vol->mat, pnt);
	goto vol_kpt_end;
    }

    // No keystr matches - failed
    return NULL;

vol_kpt_end:

    MAT4X3PNT(*pt, mat, mpt);

    return k;
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
