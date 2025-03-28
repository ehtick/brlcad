/*                          P N T S . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2025 United States Government as represented by
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
/** @file primitives/pnts/pnts.c
 *
 * Collection of points.
 *
 */

#include "common.h"

/* system headers */
#include "bnetwork.h"

/* common headers */
#include "bu/cv.h"
#include "bn.h"

#include "raytrace.h"
#include "rt/geom.h"
#include "vmath.h"


extern int rt_ell_plot(struct bu_list *, struct rt_db_internal *, const struct bg_tess_tol *, const struct bn_tol *, const struct bview *);


static unsigned char *
pnts_pack_double(unsigned char *buf, unsigned char *data, unsigned int count)
{
    bu_cv_htond(buf, data, count);
    buf += count * SIZEOF_NETWORK_DOUBLE;
    return buf;
}


static unsigned char *
pnts_unpack_double(unsigned char *buf, unsigned char *data, unsigned int count)
{
    bu_cv_ntohd(data, buf, count);
    buf += count * SIZEOF_NETWORK_DOUBLE;
    return buf;
}

static void
_pnts_calc_bbox(point_t *min, point_t *max, point_t *v, fastf_t scale)
{
    point_t sph_min, sph_max;
    if (!min || !max || !v) return;
    if (scale > 0) {
	/* we're making spheres out of these, so the bbox
	 * has to take that into account */
	sph_min[X] = (*v)[X] - scale;
	sph_max[X] = (*v)[X] + scale;
	sph_min[Y] = (*v)[Y] - scale;
	sph_max[Y] = (*v)[Y] + scale;
	sph_min[Z] = (*v)[Z] - scale;
	sph_max[Z] = (*v)[Z] + scale;
	VMINMAX((*min), (*max), sph_min);
	VMINMAX((*min), (*max), sph_max);

    } else {
	VMINMAX((*min), (*max), (*v));
    }
}


/**
 * Calculate a bounding box for a set of points
 */
int
rt_pnts_bbox(struct rt_db_internal *ip, point_t *min, point_t *max, const struct bn_tol *UNUSED(tol)) {
    struct rt_pnts_internal *pnts;
    struct bu_list *head;
    struct pnt *point;
    struct pnt_color *point_color;
    struct pnt_scale *point_scale;
    struct pnt_normal *point_normal;
    struct pnt_color_scale *point_color_scale;
    struct pnt_color_normal *point_color_normal;
    struct pnt_scale_normal *point_scale_normal;
    struct pnt_color_scale_normal *point_color_scale_normal;

    RT_CK_DB_INTERNAL(ip);
    pnts = (struct rt_pnts_internal *)ip->idb_ptr;
    RT_PNTS_CK_MAGIC(pnts);

    if (pnts->count <= 0) {
	return 0;
    }

    VSETALL((*min), INFINITY);
    VSETALL((*max), -INFINITY);

    switch (pnts->type) {
	case RT_PNT_TYPE_PNT:
	    point = (struct pnt *)pnts->point;
	    head = &point->l;
	    for (BU_LIST_FOR(point, pnt, head)) {
		_pnts_calc_bbox(min, max, &(point->v), pnts->scale);
	    }
	    break;
	case RT_PNT_TYPE_COL:
	    point_color = (struct pnt_color *)pnts->point;
	    head = &point_color->l;
	    for (BU_LIST_FOR(point_color, pnt_color, head)) {
		_pnts_calc_bbox(min, max, &(point_color->v), pnts->scale);
	    }
	    break;
	case RT_PNT_TYPE_SCA:
	    point_scale = (struct pnt_scale *)pnts->point;
	    head = &point_scale->l;
	    for (BU_LIST_FOR(point_scale, pnt_scale, head)) {
		_pnts_calc_bbox(min, max, &(point_scale->v), pnts->scale);
	    }
	    break;
	case RT_PNT_TYPE_NRM:
	    point_normal = (struct pnt_normal *)pnts->point;
	    head = &point_normal->l;
	    for (BU_LIST_FOR(point_normal, pnt_normal, head)) {
		_pnts_calc_bbox(min, max, &(point_normal->v), pnts->scale);
	    }
	    break;
	case RT_PNT_TYPE_COL_SCA:
	    point_color_scale = (struct pnt_color_scale *)pnts->point;
	    head = &point_color_scale->l;
	    for (BU_LIST_FOR(point_color_scale, pnt_color_scale, head)) {
		_pnts_calc_bbox(min, max, &(point_color_scale->v), pnts->scale);
	    }
	    break;
	case RT_PNT_TYPE_COL_NRM:
	    point_color_normal = (struct pnt_color_normal *)pnts->point;
	    head = &point_color_normal->l;
	    for (BU_LIST_FOR(point_color_normal, pnt_color_normal, head)) {
		_pnts_calc_bbox(min, max, &(point_color_normal->v), pnts->scale);
	    }
	    break;
	case RT_PNT_TYPE_SCA_NRM:
	    point_scale_normal = (struct pnt_scale_normal *)pnts->point;
	    head = &point_scale_normal->l;
	    for (BU_LIST_FOR(point_scale_normal, pnt_scale_normal, head)) {
		_pnts_calc_bbox(min, max, &(point_scale_normal->v), pnts->scale);
	    }
	    break;
	case RT_PNT_TYPE_COL_SCA_NRM:
	    point_color_scale_normal = (struct pnt_color_scale_normal *)pnts->point;
	    head = &point_color_scale_normal->l;
	    for (BU_LIST_FOR(point_color_scale_normal, pnt_color_scale_normal, head)) {
		_pnts_calc_bbox(min, max, &(point_color_scale_normal->v), pnts->scale);
	    }
	    break;
	default:
	    break;
    }

    return 0;
}

int
rt_pnts_prep(struct soltab *stp, struct rt_db_internal *ip, struct rt_i *rtip)
{
    struct rt_pnts_internal *pnts_ip;

    RT_CK_DB_INTERNAL(ip);
    pnts_ip = (struct rt_pnts_internal *)ip->idb_ptr;
    RT_PNTS_CK_MAGIC(pnts_ip);

    if (rt_pnts_bbox(ip, &(stp->st_min), &(stp->st_max), &(rtip->rti_tol))) return 1;

    /* Compute bounding sphere which contains the bounding RPP.*/
    {
	vect_t work;
	register fastf_t f;

	VADD2SCALE(stp->st_center, stp->st_min, stp->st_max, 0.5);
	VSUB2SCALE(work, stp->st_max, stp->st_min, 0.5);

	f = work[X];
	if (work[Y] > f) f = work[Y];
	if (work[Z] > f) f = work[Z];
	stp->st_aradius = f;
	stp->st_bradius = MAGNITUDE(work);
    }

    return 0;
}

/**
 * Export a pnts collection from the internal structure to the
 * database format
 */
int
rt_pnts_export5(struct bu_external *external, const struct rt_db_internal *internal, double local2mm, const struct db_i *dbip)
{
    struct rt_pnts_internal *pnts = NULL;
    unsigned long pointDataSize;
    unsigned char *buf = NULL;

    /* must be double for import and export */
    double scan;

    if (dbip) RT_CK_DBI(dbip);

    /* acquire internal pnts structure */
    RT_CK_DB_INTERNAL(internal);
    BU_CK_EXTERNAL(external);
    external->ext_nbytes = 0;

    pnts = (struct rt_pnts_internal *) internal->idb_ptr;
    RT_PNTS_CK_MAGIC(pnts);

    /* allocate enough for the header (scale + type + count) */
    external->ext_nbytes = SIZEOF_NETWORK_DOUBLE + SIZEOF_NETWORK_SHORT + SIZEOF_NETWORK_LONG;
    external->ext_buf = (uint8_t *) bu_calloc(sizeof(unsigned char), external->ext_nbytes, "pnts external");
    buf = (unsigned char *)external->ext_buf;

    scan = pnts->scale; /* convert fastf_t to double */
    bu_cv_htond(buf, (unsigned char *)&scan, 1);
    buf += SIZEOF_NETWORK_DOUBLE;
    *(uint16_t *)buf = htons((unsigned short)pnts->type);
    buf += SIZEOF_NETWORK_SHORT;
    *(uint32_t *)buf = htonl(pnts->count);

    if (pnts->count <= 0) {
	/* no points to stash, we're done */
	return 0;
    }

    /* figure out how much data there is for each point */
    pointDataSize = 3; /* v */
    if (pnts->type & RT_PNT_TYPE_COL) {
	pointDataSize += 3; /* c */
    }
    if (pnts->type & RT_PNT_TYPE_SCA) {
	pointDataSize += 1; /* s */
    }
    if (pnts->type & RT_PNT_TYPE_NRM) {
	pointDataSize += 3; /* n */
    }

    /* convert number of doubles to number of network bytes required to store doubles */
    pointDataSize = pointDataSize * SIZEOF_NETWORK_DOUBLE;

    external->ext_buf = (uint8_t *)bu_realloc(external->ext_buf, external->ext_nbytes + (pnts->count * pointDataSize), "pnts external realloc");
    buf = (unsigned char *)external->ext_buf + external->ext_nbytes;
    external->ext_nbytes = external->ext_nbytes + (pnts->count * pointDataSize);

    /* get busy, serialize the point data depending on what type of point it is */
    switch (pnts->type) {
	case RT_PNT_TYPE_PNT: {
	    register struct pnt *point;

	    for (BU_LIST_FOR(point, pnt, &(((struct pnt *)pnts->point)->l))) {
		double v[3];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL: {
	    register struct pnt_color *point;

	    for (BU_LIST_FOR(point, pnt_color, &(((struct pnt_color *)pnts->point)->l))) {
		double v[3];
		double c[3];
		fastf_t cf[3];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);

		/* pack c */
		bu_color_to_rgb_floats(&point->c, cf);
		VMOVE(c, cf);
		buf = pnts_pack_double(buf, (unsigned char *)c, 3);
	    }

	    break;
	}
	case RT_PNT_TYPE_SCA: {
	    register struct pnt_scale *point;

	    for (BU_LIST_FOR(point, pnt_scale, &(((struct pnt_scale *)pnts->point)->l))) {
		double v[3];
		double s[1];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);

		/* pack s */
		s[0] = point->s * local2mm;
		buf = pnts_pack_double(buf, (unsigned char *)s, 1);
	    }

	    break;
	}
	case RT_PNT_TYPE_NRM: {
	    register struct pnt_normal *point;

	    for (BU_LIST_FOR(point, pnt_normal, &(((struct pnt_normal *)pnts->point)->l))) {
		double v[3];
		double n[3];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);

		/* pack n */
		VSCALE(n, point->n, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_SCA: {
	    register struct pnt_color_scale *point;

	    for (BU_LIST_FOR(point, pnt_color_scale, &(((struct pnt_color_scale *)pnts->point)->l))) {
		double v[3];
		double c[3];
		fastf_t cf[3];
		double s[1];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);

		/* pack c */
		bu_color_to_rgb_floats(&point->c, cf);
		VMOVE(c, cf);
		buf = pnts_pack_double(buf, (unsigned char *)c, 3);

		/* pack s */
		s[0] = point->s * local2mm;
		buf = pnts_pack_double(buf, (unsigned char *)s, 1);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_NRM: {
	    register struct pnt_color_normal *point;

	    for (BU_LIST_FOR(point, pnt_color_normal, &(((struct pnt_color_normal *)pnts->point)->l))) {
		double v[3];
		double c[3];
		fastf_t cf[3];
		double n[3];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);

		/* pack c */
		bu_color_to_rgb_floats(&point->c, cf);
		VMOVE(c, cf);
		buf = pnts_pack_double(buf, (unsigned char *)c, 3);

		/* pack n */
		VSCALE(n, point->n, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
	    }

	    break;
	}
	case RT_PNT_TYPE_SCA_NRM: {
	    register struct pnt_scale_normal *point;

	    for (BU_LIST_FOR(point, pnt_scale_normal, &(((struct pnt_scale_normal *)pnts->point)->l))) {
		double v[3];
		double s[1];
		double n[3];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);

		/* pack s */
		s[0] = point->s * local2mm;
		buf = pnts_pack_double(buf, (unsigned char *)s, 1);

		/* pack n */
		VSCALE(n, point->n, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_SCA_NRM: {
	    register struct pnt_color_scale_normal *point;

	    for (BU_LIST_FOR(point, pnt_color_scale_normal, &(((struct pnt_color_scale_normal *)pnts->point)->l))) {
		double v[3];
		double s[1];
		double c[3];
		fastf_t cf[3];
		double n[3];

		/* pack v */
		VSCALE(v, point->v, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);

		/* pack c */
		bu_color_to_rgb_floats(&point->c, cf);
		VMOVE(c, cf);
		buf = pnts_pack_double(buf, (unsigned char *)c, 3);

		/* pack s */
		s[0] = point->s * local2mm;
		buf = pnts_pack_double(buf, (unsigned char *)s, 1);

		/* pack n */
		VSCALE(n, point->n, local2mm);
		buf = pnts_pack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
	    }

	    break;
	}
	default:
	    bu_log("ERROR: unknown points primitive type (type=%d)\n", pnts->type);
	    return 0;
    }

    return 0;
}

int
rt_pnts_mat(struct rt_db_internal *rop, const mat_t mat, const struct rt_db_internal *ip)
{
    if (!rop || !mat)
	return BRLCAD_OK;

    // For the moment, we only support applying a mat to a pnts primitive in place - the
    // input and output must be the same.
    if (ip && rop != ip) {
	bu_log("rt_pnts_mat:  alignment of points between multiple pnts objects is unsupported - input pnts must be the same as the output pnts.\n");
	return BRLCAD_ERROR;
    }

    struct rt_pnts_internal *pnts = (struct rt_pnts_internal *)rop->idb_ptr;
    RT_PNTS_CK_MAGIC(pnts);

    point_t v;
    vect_t n;
    switch (pnts->type) {
	case RT_PNT_TYPE_PNT: {
	    struct pnt *point;
	    for (BU_LIST_FOR(point, pnt, &(((struct pnt *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
	    }
	    return BRLCAD_OK;
	}
	case RT_PNT_TYPE_COL: {
	    struct pnt_color *point;
	    for (BU_LIST_FOR(point, pnt_color, &(((struct pnt_color *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
	    }
	    return BRLCAD_OK;
	}
	case RT_PNT_TYPE_SCA: {
	    struct pnt_scale *point;
	    for (BU_LIST_FOR(point, pnt_scale, &(((struct pnt_scale *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
	    }
	    return BRLCAD_OK;
	}
	case RT_PNT_TYPE_NRM: {
	    struct pnt_normal *point;
	    for (BU_LIST_FOR(point, pnt_normal, &(((struct pnt_normal *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
		VMOVE(n, point->n);
		MAT4X3PNT(point->n, mat, n);
	    }
	    return BRLCAD_OK;
	}
	case RT_PNT_TYPE_COL_SCA: {
	    struct pnt_color_scale *point;
	    for (BU_LIST_FOR(point, pnt_color_scale, &(((struct pnt_color_scale *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
	    }
	    return BRLCAD_OK;
	}
	case RT_PNT_TYPE_COL_NRM: {
	    struct pnt_color_normal *point;
	    for (BU_LIST_FOR(point, pnt_color_normal, &(((struct pnt_color_normal *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
		VMOVE(n, point->n);
		MAT4X3PNT(point->n, mat, n);
	    }
	    return BRLCAD_OK;
	}
	case RT_PNT_TYPE_SCA_NRM: {
	    struct pnt_scale_normal *point;
	    for (BU_LIST_FOR(point, pnt_scale_normal, &(((struct pnt_scale_normal *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
		VMOVE(n, point->n);
		MAT4X3PNT(point->n, mat, n);
	    }
	    return BRLCAD_OK;
	}
	case RT_PNT_TYPE_COL_SCA_NRM: {
	    struct pnt_color_scale_normal *point;
	    for (BU_LIST_FOR(point, pnt_color_scale_normal, &(((struct pnt_color_scale_normal *)pnts->point)->l))) {
		VMOVE(v, point->v);
		MAT4X3PNT(point->v, mat, v);
		VMOVE(n, point->n);
		MAT4X3PNT(point->n, mat, n);
	    }
	    return BRLCAD_OK;
	}
	default:
	    bu_log("rt_pnts_mat: unknown points primitive type (type=%d)\n", pnts->type);
	    return BRLCAD_ERROR;
    }

    // Shouldn't be reached
    return BRLCAD_ERROR;
}

/**
 * Import a pnts collection from the database format to the internal
 * structure and apply modeling transformations.
 */
int
rt_pnts_import5(struct rt_db_internal *internal, const struct bu_external *external, const fastf_t *mat, const struct db_i *dbip)
{
    struct rt_pnts_internal *pnts = NULL;
    struct bu_list *head = NULL;
    unsigned char *buf = NULL;
    unsigned long i;
    uint16_t type;

    /* must be double for import and export */
    double scan;

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(internal);
    BU_CK_EXTERNAL(external);
    buf = (unsigned char *)external->ext_buf;

    /* initialize database structure */
    internal->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    internal->idb_type = ID_PNTS;
    internal->idb_meth = &OBJ[ID_PNTS];
    BU_ALLOC(internal->idb_ptr, struct rt_pnts_internal);

    /* initialize internal structure */
    pnts = (struct rt_pnts_internal *) internal->idb_ptr;
    pnts->magic = RT_PNTS_INTERNAL_MAGIC;
    pnts->point = NULL;

    /* unpack the header */
    bu_cv_ntohd((unsigned char *)&scan, buf, 1);
    pnts->scale = scan; /* convert double to fastf_t */
    buf += SIZEOF_NETWORK_DOUBLE;
    type = ntohs(*(uint16_t *)buf);
    pnts->type = (rt_pnt_type)type; /* intentional enum coercion */
    buf += SIZEOF_NETWORK_SHORT;
    pnts->count = ntohl(*(uint32_t *)buf);
    buf += SIZEOF_NETWORK_LONG;

    if (pnts->count <= 0) {
	/* no points to read, we're done */
	return 0;
    }


    /* get busy, deserialize the point data depending on what type of point it is */
    switch (pnts->type) {
	case RT_PNT_TYPE_PNT: {
	    register struct pnt *point;

	    BU_ALLOC(point, struct pnt);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];

		BU_ALLOC(point, struct pnt);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL: {
	    register struct pnt_color *point;

	    BU_ALLOC(point, struct pnt_color);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];
		double c[3];
		fastf_t cf[3];

		BU_ALLOC(point, struct pnt_color);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		/* unpack c */
		buf = pnts_unpack_double(buf, (unsigned char *)c, 3);
		VMOVE(cf, c);
		bu_color_from_rgb_floats(&point->c, cf);

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	case RT_PNT_TYPE_SCA: {
	    register struct pnt_scale *point;

	    BU_ALLOC(point, struct pnt_scale);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];
		double s[1];

		BU_ALLOC(point, struct pnt_scale);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		/* unpack s */
		buf = pnts_unpack_double(buf, (unsigned char *)s, 1);
		point->s = s[0];

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	case RT_PNT_TYPE_NRM: {
	    register struct pnt_normal *point;

	    BU_ALLOC(point, struct pnt_normal);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];
		double n[3];

		BU_ALLOC(point, struct pnt_normal);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		/* unpack n */
		buf = pnts_unpack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
		VMOVE(point->n, n);

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_SCA: {
	    register struct pnt_color_scale *point;

	    BU_ALLOC(point, struct pnt_color_scale);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];
		double c[3];
		fastf_t cf[3];
		double s[1];

		BU_ALLOC(point, struct pnt_color_scale);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		/* unpack c */
		buf = pnts_unpack_double(buf, (unsigned char *)c, 3);
		VMOVE(cf, c);
		bu_color_from_rgb_floats(&point->c, cf);

		/* unpack s */
		buf = pnts_unpack_double(buf, (unsigned char *)s, 1);
		point->s = s[0];

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_NRM: {
	    register struct pnt_color_normal *point;

	    BU_ALLOC(point, struct pnt_color_normal);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];
		double c[3];
		fastf_t cf[3];
		double n[3];

		BU_ALLOC(point, struct pnt_color_normal);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		/* unpack c */
		buf = pnts_unpack_double(buf, (unsigned char *)c, 3);
		VMOVE(cf, c);
		bu_color_from_rgb_floats(&point->c, cf);

		/* unpack n */
		buf = pnts_unpack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
		VMOVE(point->n, n);

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	case RT_PNT_TYPE_SCA_NRM: {
	    register struct pnt_scale_normal *point;

	    BU_ALLOC(point, struct pnt_scale_normal);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];
		double s[1];
		double n[3];

		BU_ALLOC(point, struct pnt_scale_normal);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		/* unpack s */
		buf = pnts_unpack_double(buf, (unsigned char *)s, 1);
		point->s = s[0];

		/* unpack n */
		buf = pnts_unpack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
		VMOVE(point->n, n);

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_SCA_NRM: {
	    register struct pnt_color_scale_normal *point;

	    BU_ALLOC(point, struct pnt_color_scale_normal);
	    head = &point->l;
	    BU_LIST_INIT(head);
	    pnts->point = point;

	    for (i = 0; i < pnts->count; i++) {
		double v[3];
		double s[1];
		double c[3];
		fastf_t cf[3];
		double n[3];

		BU_ALLOC(point, struct pnt_color_scale_normal);

		/* unpack v */
		buf = pnts_unpack_double(buf, (unsigned char *)v, ELEMENTS_PER_POINT);
		VMOVE(point->v, v);

		/* unpack c */
		buf = pnts_unpack_double(buf, (unsigned char *)c, 3);
		VMOVE(cf, c);
		bu_color_from_rgb_floats(&point->c, cf);

		/* unpack s */
		buf = pnts_unpack_double(buf, (unsigned char *)s, 1);
		point->s = s[0];

		/* unpack n */
		buf = pnts_unpack_double(buf, (unsigned char *)n, ELEMENTS_PER_VECT);
		VMOVE(point->n, n);

		BU_LIST_PUSH(head, &point->l);
	    }

	    break;
	}
	default:
	    bu_log("ERROR: unknown points primitive type (type=%d)\n", pnts->type);
	    return 0;
    }

    /* Apply transform */
    if (mat == NULL) mat = bn_mat_identity;
    return rt_pnts_mat(internal, mat, NULL);
}


/**
 * Free the storage associated with the rt_db_internal version of the
 * collection.  This uses type aliasing to iterate over the list of
 * points as a bu_list instead of calling up a switching table for
 * each point type.
 */
void
rt_pnts_ifree(struct rt_db_internal *internal)
{
    struct rt_pnts_internal *pnts;
    register struct bu_list *point;

    RT_CK_DB_INTERNAL(internal);

    pnts = ((struct rt_pnts_internal *)(internal->idb_ptr));
    RT_PNTS_CK_MAGIC(pnts);

    if (pnts->count == 0) {
	return;
    }

    /* since each point type has a bu_list as the first struct
     * element, we can treat them all as 'pnt' structs in order to
     * iterate over the bu_list and free them.
     */
    while (BU_LIST_WHILE(point, bu_list, &(((struct pnt *)pnts->point)->l))) {
	BU_LIST_DEQUEUE(point);
	bu_free(point, "free point");
    }

    /* free the head point */
    bu_free(pnts->point, "free head point");
    pnts->point = NULL; /* sanity */

    /* free the internal container */
    bu_free(internal->idb_ptr, "pnts ifree");
    internal->idb_ptr = ((void *)0); /* sanity */
}


void
rt_pnts_print(register const struct soltab *stp)
{
    register struct rt_pnts_internal *pnts;

    pnts = (struct rt_pnts_internal *)stp->st_specific;
    RT_PNTS_CK_MAGIC(pnts);

    switch (pnts->type) {
	case RT_PNT_TYPE_PNT: {
	    register struct pnt *point;
	    for (BU_LIST_FOR(point, pnt, &(((struct pnt *)pnts->point)->l))) {
	    }

	    break;
	}
	case RT_PNT_TYPE_COL: {
	    register struct pnt_color *point;
	    for (BU_LIST_FOR(point, pnt_color, &(((struct pnt_color *)pnts->point)->l))) {
	    }

	    break;
	}
	case RT_PNT_TYPE_SCA: {
	    register struct pnt_scale *point;
	    for (BU_LIST_FOR(point, pnt_scale, &(((struct pnt_scale *)pnts->point)->l))) {
	    }

	    break;
	}
	case RT_PNT_TYPE_NRM: {
	    register struct pnt_normal *point;
	    for (BU_LIST_FOR(point, pnt_normal, &(((struct pnt_normal *)pnts->point)->l))) {
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_SCA: {
	    register struct pnt_color_scale *point;
	    for (BU_LIST_FOR(point, pnt_color_scale, &(((struct pnt_color_scale *)pnts->point)->l))) {
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_NRM: {
	    register struct pnt_color_normal *point;
	    for (BU_LIST_FOR(point, pnt_color_normal, &(((struct pnt_color_normal *)pnts->point)->l))) {
	    }

	    break;
	}
	case RT_PNT_TYPE_SCA_NRM: {
	    register struct pnt_scale_normal *point;
	    for (BU_LIST_FOR(point, pnt_scale_normal, &(((struct pnt_scale_normal *)pnts->point)->l))) {
	    }

	    break;
	}
	case RT_PNT_TYPE_COL_SCA_NRM: {
	    register struct pnt_color_scale_normal *point;
	    for (BU_LIST_FOR(point, pnt_color_scale_normal, &(((struct pnt_color_scale_normal *)pnts->point)->l))) {
	    }

	    break;
	}
	default:
	    bu_log("ERROR: unknown points primitive type (type=%d)\n", pnts->type);
    }
}


/**
 * Plot pnts collection as axes or spheres.
 */
int
rt_pnts_plot(struct bu_list *vhead, struct rt_db_internal *internal, const struct bg_tess_tol *ttol, const struct bn_tol *tol, const struct bview *UNUSED(info))
{
    struct rt_pnts_internal *pnts;
    struct bu_list *head;
    struct rt_db_internal db;
    struct rt_ell_internal ell;
    struct pnt *point;
    double scale;
    point_t a, b;

    BU_CK_LIST_HEAD(vhead);
    RT_CK_DB_INTERNAL(internal);

    struct bu_list *vlfree = &rt_vlfree;
    pnts = (struct rt_pnts_internal *)internal->idb_ptr;
    RT_PNTS_CK_MAGIC(pnts);

    if (pnts->count > 0) {
	point = (struct pnt *)pnts->point;
	head = &point->l;
	scale = pnts->scale;
    } else {
	return 0;
    }

    if (scale > 0) {
	/* set local database */
	db.idb_magic = RT_DB_INTERNAL_MAGIC;
	db.idb_major_type = ID_ELL;
	db.idb_ptr = &ell;

	/* set local ell for the pnts collection */
	ell.magic = RT_ELL_INTERNAL_MAGIC;

	VSET(ell.a, scale, 0, 0);
	VSET(ell.b, 0, scale, 0);
	VSET(ell.c, 0, 0, scale);

	/* give rt_ell_plot a sphere representation of each point */
	for (BU_LIST_FOR(point, pnt, head)) {
	    VMOVE(ell.v, point->v);
	    rt_ell_plot(vhead, &db, ttol, tol, NULL);
	}
    } else {
	double vCoord, hCoord;
	vCoord = hCoord = 1;

	for (BU_LIST_FOR(point, pnt, head)) {
	    /* draw first horizontal segment for this point */
	    VSET(a, point->v[X] - hCoord, point->v[Y], point->v[Z]);
	    VSET(b, point->v[X] + hCoord, point->v[Y], point->v[Z]);
	    BV_ADD_VLIST(vlfree, vhead, a, BV_VLIST_LINE_MOVE);
	    BV_ADD_VLIST(vlfree, vhead, b, BV_VLIST_LINE_DRAW);

	    /* draw perpendicular horizontal segment */
	    VSET(a, point->v[X], point->v[Y] - hCoord, point->v[Z]);
	    VSET(b, point->v[X], point->v[Y] + hCoord, point->v[Z]);
	    BV_ADD_VLIST(vlfree, vhead, a, BV_VLIST_LINE_MOVE);
	    BV_ADD_VLIST(vlfree, vhead, b, BV_VLIST_LINE_DRAW);

	    /* draw vertical segment */
	    VSET(a, point->v[X], point->v[Y], point->v[Z] - vCoord);
	    VSET(b, point->v[X], point->v[Y], point->v[Z] + vCoord);
	    BV_ADD_VLIST(vlfree, vhead, a, BV_VLIST_LINE_MOVE);
	    BV_ADD_VLIST(vlfree, vhead, b, BV_VLIST_LINE_DRAW);
	}
    }

    return 0;
}


/**
 * Make human-readable formatted presentation of this primitive.  First
 * line describes type of solid.  Additional lines are indented one
 * tab, and give parameter values.
 */
int
rt_pnts_describe(struct bu_vls *str, const struct rt_db_internal *intern, int verbose, double mm2local)
{
    const struct rt_pnts_internal *pnts;
    double defaultSize = 0.0;
    unsigned long numPoints = 0;
    unsigned long loop_counter = 0;

    char buf[256]= {0};
    static const int BUF_SZ = 256;

    /* retrieve head record values */
    pnts = (struct rt_pnts_internal *) intern->idb_ptr;
    RT_PNTS_CK_MAGIC(pnts);

    defaultSize = pnts->scale;
    numPoints = pnts->count;

    bu_vls_strcat(str, "Point Cloud (PNTS)\n");

    /* NOTE: this value is not arbitrary, but is dependent on the init in libged/list/list.c */
    if (verbose <= 99) {
	bu_vls_strcat(str, "    use -v (verbose) for all data\n");
	return 1;
    }

    snprintf(buf, BUF_SZ, "Total number of points: %lu\nDefault scale: %f\n", numPoints, defaultSize);
    bu_vls_strcat(str, buf);

    if (pnts->count == 0) {
	return 0;
    }

    loop_counter = 1;
    switch (pnts->type) {
	case RT_PNT_TYPE_PNT: {
	    register struct pnt *point;
	    bu_vls_strcat(str, "point#, (point)\n");
	    for (BU_LIST_FOR(point, pnt, &(((struct pnt *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	case RT_PNT_TYPE_COL: {
	    register struct pnt_color *point;
	    bu_vls_strcat(str, "point#, (point), (color)\n");
	    for (BU_LIST_FOR(point, pnt_color, &(((struct pnt_color *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f), (%f %f %f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local,
			 point->c.buc_rgb[0],
			 point->c.buc_rgb[1],
			 point->c.buc_rgb[2]);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	case RT_PNT_TYPE_SCA: {
	    register struct pnt_scale *point;
	    bu_vls_strcat(str, "point#, (point), (scale)\n");
	    for (BU_LIST_FOR(point, pnt_scale, &(((struct pnt_scale *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f), (%f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local,
			 point->s);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	case RT_PNT_TYPE_NRM: {
	    register struct pnt_normal *point;
	    bu_vls_strcat(str, "point#, (point), (normal)\n");
	    for (BU_LIST_FOR(point, pnt_normal, &(((struct pnt_normal *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f), (%f %f %f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local,
			 point->n[X],
			 point->n[Y],
			 point->n[Z]);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	case RT_PNT_TYPE_COL_SCA: {
	    register struct pnt_color_scale *point;
	    bu_vls_strcat(str, "point#, (point), (color), (scale)\n");
	    for (BU_LIST_FOR(point, pnt_color_scale, &(((struct pnt_color_scale *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f), (%f %f %f), (%f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local,
			 point->c.buc_rgb[0],
			 point->c.buc_rgb[1],
			 point->c.buc_rgb[2],
			 point->s);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	case RT_PNT_TYPE_COL_NRM: {
	    register struct pnt_color_normal *point;
	    bu_vls_strcat(str, "point#, (point), (color), (normal)\n");
	    for (BU_LIST_FOR(point, pnt_color_normal, &(((struct pnt_color_normal *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f), (%f %f %f), (%f %f %f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local,
			 point->c.buc_rgb[0],
			 point->c.buc_rgb[1],
			 point->c.buc_rgb[2],
			 point->n[X],
			 point->n[Y],
			 point->n[Z]);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	case RT_PNT_TYPE_SCA_NRM: {
	    register struct pnt_scale_normal *point;
	    bu_vls_strcat(str, "point#, (point), (scale), (normal)\n");
	    for (BU_LIST_FOR(point, pnt_scale_normal, &(((struct pnt_scale_normal *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f), (%f), (%f %f %f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local,
			 point->s,
			 point->n[X],
			 point->n[Y],
			 point->n[Z]);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	case RT_PNT_TYPE_COL_SCA_NRM: {
	    register struct pnt_color_scale_normal *point;
	    bu_vls_strcat(str, "point#, (point), (color), (scale), (normal)\n");
	    for (BU_LIST_FOR(point, pnt_color_scale_normal, &(((struct pnt_color_scale_normal *)pnts->point)->l))) {
		snprintf(buf, BUF_SZ, "%lu, \t (%f %f %f), (%f %f %f), (%f), (%f %f %f)\n",
			 (long unsigned)loop_counter,
			 point->v[X] * mm2local,
			 point->v[Y] * mm2local,
			 point->v[Z] * mm2local,
			 point->c.buc_rgb[0],
			 point->c.buc_rgb[1],
			 point->c.buc_rgb[2],
			 point->s,
			 point->n[X],
			 point->n[Y],
			 point->n[Z]);
		bu_vls_strcat(str, buf);
		loop_counter++;
	    }
	    break;
	}
	default:
	    bu_log("ERROR: unknown points primitive type (type=%d)\n", pnts->type);
	    return 1;
    }

    return 0;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
