/*                      E D M E T A B A L L . C
 * BRL-CAD
 *
 * Copyright (c) 1996-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file primitives/edmetaball.c
 *
 */

#include "common.h"

#include <math.h>
#include <string.h>

#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "rt/geom.h"
#include "wdb.h"

#include "../edit_private.h"

#define ECMD_METABALL_PT_NEXT		30121
#define ECMD_METABALL_PT_PREV		30122
#define ECMD_METABALL_PT_ADD		36089	/* add a metaball control point */
#define ECMD_METABALL_PT_DEL		36088	/* delete a metaball control point */
#define ECMD_METABALL_PT_FLDSTR		36087	/* set a metaball control point field strength */
#define ECMD_METABALL_PT_MOV		36086	/* move a metaball control point */
#define ECMD_METABALL_PT_PICK		36085	/* pick a metaball control point */
#define ECMD_METABALL_PT_SET_GOO	30119
#define ECMD_METABALL_RMET		36090	/* set the metaball render method */
#define ECMD_METABALL_SET_METHOD	36084	/* set the rendering method */
#define ECMD_METABALL_SET_THRESHOLD	36083	/* overall metaball threshold value */

struct rt_metaball_edit {
    struct wdb_metaball_pnt *es_metaball_pnt; /* Currently selected METABALL Point */
};

void *
rt_edit_metaball_prim_edit_create(struct rt_edit *UNUSED(s))
{
    struct rt_metaball_edit *m;
    BU_GET(m, struct rt_metaball_edit);

    m->es_metaball_pnt = NULL;

    return (void *)m;
}

void
rt_edit_metaball_prim_edit_destroy(struct rt_metaball_edit *m)
{
    if (!m)
	return;

    // Sanity
    m->es_metaball_pnt = NULL;

    BU_PUT(m, struct rt_metaball_edit);
}

void
rt_edit_metaball_prim_edit_reset(struct rt_edit *s)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    m->es_metaball_pnt = NULL;
}

void
rt_edit_metaball_set_edit_mode(struct rt_edit *s, int mode)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    struct wdb_metaball_pnt *next, *prev;

    rt_edit_set_edflag(s, mode);

    switch (mode) {
	case ECMD_METABALL_SET_THRESHOLD:
	case ECMD_METABALL_SET_METHOD:
	case ECMD_METABALL_PT_SET_GOO:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
	    break;
	case ECMD_METABALL_PT_PICK:
	    s->edit_mode = RT_PARAMS_EDIT_PICK;
	    break;
	case ECMD_METABALL_PT_NEXT:
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		return;
	    }
	    next = BU_LIST_NEXT(wdb_metaball_pnt, &m->es_metaball_pnt->l);
	    if (next->l.magic == BU_LIST_HEAD_MAGIC) {
		bu_vls_printf(s->log_str, "Current point is the last\n");
		return;
	    }
	    m->es_metaball_pnt = next;
	    rt_metaball_pnt_print(m->es_metaball_pnt, s->base2local);
	    rt_edit_set_edflag(s, RT_EDIT_IDLE);
	    break;
	case ECMD_METABALL_PT_PREV:
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		return;
	    }
	    prev = BU_LIST_PREV(wdb_metaball_pnt, &m->es_metaball_pnt->l);
	    if (prev->l.magic == BU_LIST_HEAD_MAGIC) {
		bu_vls_printf(s->log_str, "Current point is the first\n");
		return;
	    }
	    m->es_metaball_pnt = prev;
	    rt_metaball_pnt_print(m->es_metaball_pnt, s->base2local);
	    rt_edit_set_edflag(s, RT_EDIT_IDLE);
	    break;
	case ECMD_METABALL_PT_MOV:
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		rt_edit_set_edflag(s, RT_EDIT_IDLE);
		return;
	    }
	    break;
	case ECMD_METABALL_PT_FLDSTR:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		rt_edit_set_edflag(s, RT_EDIT_IDLE);
		return;
	    }
	    break;
	case ECMD_METABALL_PT_DEL:
	case ECMD_METABALL_PT_ADD:
	    break;
    }

    bu_clbk_t f = NULL;
    void *d = NULL;
    int flag = 1;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_EAXES_POS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, &flag);
}

static void
metaball_ed(struct rt_edit *s, int arg, int UNUSED(a), int UNUSED(b), void *UNUSED(data))
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    struct wdb_metaball_pnt *next, *prev;

    rt_edit_set_edflag(s, arg);

    switch (arg) {
	case ECMD_METABALL_SET_THRESHOLD:
	case ECMD_METABALL_SET_METHOD:
	case ECMD_METABALL_PT_SET_GOO:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
	    break;
	case ECMD_METABALL_PT_PICK:
	    s->edit_mode = RT_PARAMS_EDIT_PICK;
	    break;
	case ECMD_METABALL_PT_NEXT:
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		return;
	    }
	    next = BU_LIST_NEXT(wdb_metaball_pnt, &m->es_metaball_pnt->l);
	    if (next->l.magic == BU_LIST_HEAD_MAGIC) {
		bu_vls_printf(s->log_str, "Current point is the last\n");
		return;
	    }
	    m->es_metaball_pnt = next;
	    rt_metaball_pnt_print(m->es_metaball_pnt, s->base2local);
	    rt_edit_set_edflag(s, RT_EDIT_IDLE);
	    // TODO - should we really be calling this here?
	    rt_edit_process(s);
	    break;
	case ECMD_METABALL_PT_PREV:
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		return;
	    }
	    prev = BU_LIST_PREV(wdb_metaball_pnt, &m->es_metaball_pnt->l);
	    if (prev->l.magic == BU_LIST_HEAD_MAGIC) {
		bu_vls_printf(s->log_str, "Current point is the first\n");
		return;
	    }
	    m->es_metaball_pnt = prev;
	    rt_metaball_pnt_print(m->es_metaball_pnt, s->base2local);
	    rt_edit_set_edflag(s, RT_EDIT_IDLE);
	    // TODO - should we really be calling this here?
	    rt_edit_process(s);
	    break;
	case ECMD_METABALL_PT_MOV:
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		rt_edit_set_edflag(s, RT_EDIT_IDLE);
		return;
	    }
	    // TODO - should we really be calling this here?
	    rt_edit_process(s);
	    break;
	case ECMD_METABALL_PT_FLDSTR:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
	    if (!m->es_metaball_pnt) {
		bu_vls_printf(s->log_str, "No Metaball Point selected\n");
		rt_edit_set_edflag(s, RT_EDIT_IDLE);
		return;
	    }
	    break;
	case ECMD_METABALL_PT_DEL:
	    // TODO - should we really be calling this here?
	    rt_edit_process(s);
	    break;
	case ECMD_METABALL_PT_ADD:
	    break;
    }

    bu_clbk_t f = NULL;
    void *d = NULL;
    int flag = 1;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_EAXES_POS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, &flag);
}

struct rt_edit_menu_item metaball_menu[] = {
    { "METABALL MENU", NULL, 0 },
    { "Set Threshold", metaball_ed, ECMD_METABALL_SET_THRESHOLD },
    { "Set Render Method", metaball_ed, ECMD_METABALL_SET_METHOD },
    { "Select Point", metaball_ed, ECMD_METABALL_PT_PICK},
    { "Next Point", metaball_ed, ECMD_METABALL_PT_NEXT },
    { "Previous Point", metaball_ed, ECMD_METABALL_PT_PREV },
    { "Move Point", metaball_ed, ECMD_METABALL_PT_MOV },
    { "Scale Point fldstr", metaball_ed, ECMD_METABALL_PT_FLDSTR },
    { "Scale Point \"goo\" value", metaball_ed, ECMD_METABALL_PT_SET_GOO },
    { "Delete Point", metaball_ed, ECMD_METABALL_PT_DEL },
    { "Add Point", metaball_ed, ECMD_METABALL_PT_ADD },
    { "", NULL, 0 }
};

struct rt_edit_menu_item *
rt_edit_metaball_menu_item(const struct bn_tol *UNUSED(tol))
{
    return metaball_menu;
}


void
rt_edit_metaball_labels(
	int *UNUSED(num_lines),
	point_t *UNUSED(lines),
	struct rt_point_labels *pl,
	int UNUSED(max_pl),
	const mat_t xform,
	struct rt_edit *s,
	struct bn_tol *UNUSED(tol))
{
    struct rt_db_internal *ip = &s->es_int;
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    point_t pos_view;
    int npl = 0;

    RT_CK_DB_INTERNAL(ip);

#define POINT_LABEL_STR(_pt, _str) { \
    VMOVE(pl[npl].pt, _pt); \
    bu_strlcpy(pl[npl++].str, _str, sizeof(pl[0].str)); }

    struct rt_metaball_internal *metaball =
	(struct rt_metaball_internal *)ip->idb_ptr;

    RT_METABALL_CK_MAGIC(metaball);

    if (m->es_metaball_pnt) {
	BU_CKMAG(m->es_metaball_pnt, WDB_METABALLPT_MAGIC, "wdb_metaball_pnt");

	MAT4X3PNT(pos_view, xform, m->es_metaball_pnt->coord);
	POINT_LABEL_STR(pos_view, "pt");
    }

    pl[npl].str[0] = '\0';	/* Mark ending */
}

const char *
rt_edit_metaball_keypoint(
	point_t *pt,
	const char *UNUSED(keystr),
	const mat_t mat,
	struct rt_edit *s,
	const struct bn_tol *UNUSED(tol))
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    struct rt_db_internal *ip = &s->es_int;
    RT_CK_DB_INTERNAL(ip);
    point_t mpt = VINIT_ZERO;
    VSETALL(mpt, 0.0);
    static char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);

    struct rt_metaball_internal *metaball = (struct rt_metaball_internal *)ip->idb_ptr;
    RT_METABALL_CK_MAGIC(metaball);

    if (m->es_metaball_pnt==NULL) {
	snprintf(buf, BUFSIZ, "no point selected");
    } else {
	VMOVE(mpt, m->es_metaball_pnt->coord);
	snprintf(buf, BUFSIZ, "V %f", m->es_metaball_pnt->fldstr);
    }

    MAT4X3PNT(*pt, mat, mpt);
    return (const char *)buf;
}

int
ecmd_metaball_set_threshold(struct rt_edit *s)
{
    if (s->e_para[0] < 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR < 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    struct rt_metaball_internal *ball =
	(struct rt_metaball_internal *)s->es_int.idb_ptr;
    RT_METABALL_CK_MAGIC(ball);
    ball->threshold = s->e_para[0];

    return 0;
}

int
ecmd_metaball_set_method(struct rt_edit *s)
{
    if (s->e_para[0] < 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR < 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    struct rt_metaball_internal *ball =
	(struct rt_metaball_internal *)s->es_int.idb_ptr;
    RT_METABALL_CK_MAGIC(ball);
    ball->method = s->e_para[0];

    return 0;
}

int
ecmd_metaball_pt_set_goo(struct rt_edit *s)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    if (s->e_para[0] < 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR < 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    if (!m->es_metaball_pnt || !s->e_inpara) {
	bu_vls_printf(s->log_str, "pscale: no metaball point selected for scaling goo\n");
	return BRLCAD_ERROR;
    }
    m->es_metaball_pnt->sweat *= *s->e_para * ((s->es_scale > -SMALL_FASTF) ? s->es_scale : 1.0);

    return 0;
}

int
ecmd_metaball_pt_fldstr(struct rt_edit *s)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    if (s->e_para[0] <= 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR <= 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    if (!m->es_metaball_pnt || !s->e_inpara) {
	bu_vls_printf(s->log_str, "pscale: no metaball point selected for scaling strength\n");
	return BRLCAD_ERROR;
    }

    m->es_metaball_pnt->fldstr *= *s->e_para * ((s->es_scale > -SMALL_FASTF) ? s->es_scale : 1.0);

    return 0;
}

void
ecmd_metaball_pt_pick(struct rt_edit *s)
{
    struct rt_metaball_internal *metaball=
	(struct rt_metaball_internal *)s->es_int.idb_ptr;
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    point_t new_pt;
    struct wdb_metaball_pnt *ps;
    struct wdb_metaball_pnt *nearest=(struct wdb_metaball_pnt *)NULL;
    fastf_t min_dist = MAX_FASTF;
    vect_t dir, work;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_METABALL_CK_MAGIC(metaball);

    if (s->e_mvalid) {
	VMOVE(new_pt, s->e_mparam);
    } else if (s->e_inpara == 3) {
	/* must convert to base units */
	s->e_para[0] *= s->local2base;
	s->e_para[1] *= s->local2base;
	s->e_para[2] *= s->local2base;
	VMOVE(new_pt, s->e_para);
    } else if (s->e_inpara) {
	bu_vls_printf(s->log_str, "x y z coordinates required for control point selection\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return;
    } else {
	return;
    }

    /* get a direction vector in model space corresponding to z-direction in view */
    VSET(work, 0.0, 0.0, 1.0);
    MAT4X3VEC(dir, s->vp->gv_view2model, work);

    for (BU_LIST_FOR(ps, wdb_metaball_pnt, &metaball->metaball_ctrl_head)) {
	fastf_t dist;

	dist = bg_dist_line3_pnt3(new_pt, dir, ps->coord);
	if (dist < min_dist) {
	    min_dist = dist;
	    nearest = ps;
	}
    }

    m->es_metaball_pnt = nearest;

    if (!m->es_metaball_pnt) {
	bu_vls_printf(s->log_str, "No METABALL control point selected\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
    } else {
	rt_metaball_pnt_print(m->es_metaball_pnt, s->base2local);
    }
}

void
ecmd_metaball_pt_mov(struct rt_edit *s)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    if (!m->es_metaball_pnt) {
	bu_log("Must select a point to move"); return; }
    if (s->e_inpara != 3) {
	bu_log("Must provide dx dy dz"); return; }
    /* must convert to base units */
    s->e_para[0] *= s->local2base;
    s->e_para[1] *= s->local2base;
    s->e_para[2] *= s->local2base;
    VADD2(m->es_metaball_pnt->coord, m->es_metaball_pnt->coord, s->e_para);
}

void
ecmd_metaball_pt_del(struct rt_edit *s)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    struct wdb_metaball_pnt *tmp = m->es_metaball_pnt, *p;

    if (m->es_metaball_pnt == NULL) {
	bu_log("No point selected");
	return;
    }
    p = BU_LIST_PREV(wdb_metaball_pnt, &m->es_metaball_pnt->l);
    if (p->l.magic == BU_LIST_HEAD_MAGIC) {
	m->es_metaball_pnt = BU_LIST_NEXT(wdb_metaball_pnt, &m->es_metaball_pnt->l);
	/* 0 point metaball... allow it for now. */
	if (m->es_metaball_pnt->l.magic == BU_LIST_HEAD_MAGIC)
	    m->es_metaball_pnt = NULL;
    } else
	m->es_metaball_pnt = p;
    BU_LIST_DQ(&tmp->l);
    free(tmp);
    if (!m->es_metaball_pnt)
	bu_log("WARNING: Last point of this metaball has been deleted.");
}

void
ecmd_metaball_pt_add(struct rt_edit *s)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    struct rt_metaball_internal *metaball= (struct rt_metaball_internal *)s->es_int.idb_ptr;
    struct wdb_metaball_pnt *n = (struct wdb_metaball_pnt *)malloc(sizeof(struct wdb_metaball_pnt));

    if (s->e_inpara != 3) {
	bu_log("Must provide x y z");
	bu_free(n, "wdb_metaball_pnt n");
	return;
    }

    /* must convert to base units */
    s->e_para[0] *= s->local2base;
    s->e_para[1] *= s->local2base;
    s->e_para[2] *= s->local2base;

    m->es_metaball_pnt = BU_LIST_FIRST(wdb_metaball_pnt, &metaball->metaball_ctrl_head);
    VMOVE(n->coord, s->e_para);
    n->l.magic = WDB_METABALLPT_MAGIC;
    n->fldstr = 1.0;
    BU_LIST_APPEND(&m->es_metaball_pnt->l, &n->l);
    m->es_metaball_pnt = n;
}

static int
rt_edit_metaball_pscale(struct rt_edit *s)
{
    if (s->e_inpara > 1) {
	bu_vls_printf(s->log_str, "ERROR: only one argument needed\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    if (s->e_inpara) {
	if (s->e_para[0] <= 0.0) {
	    bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR <= 0\n");
	    s->e_inpara = 0;
	    return BRLCAD_ERROR;
	}

	/* must convert to base units */
	s->e_para[0] *= s->local2base;
	s->e_para[1] *= s->local2base;
	s->e_para[2] *= s->local2base;
    }

    switch (s->edit_flag) {
	case ECMD_METABALL_SET_THRESHOLD:
	    return ecmd_metaball_set_threshold(s);
	case ECMD_METABALL_SET_METHOD:
	    return ecmd_metaball_set_method(s);
	case ECMD_METABALL_PT_SET_GOO:
	    return ecmd_metaball_pt_set_goo(s);
	case ECMD_METABALL_PT_FLDSTR:
	    return ecmd_metaball_pt_fldstr(s);
    };

    return 0;
}

int
rt_edit_metaball_edit(struct rt_edit *s)
{
    struct rt_metaball_edit *m = (struct rt_metaball_edit *)s->ipe_ptr;
    switch (s->edit_flag) {
	case RT_PARAMS_EDIT_SCALE:
	    /* scale the solid uniformly about its vertex point */
	    m->es_metaball_pnt = (struct wdb_metaball_pnt *)NULL; /* Reset es_metaball_pnt */
	    return edit_sscale(s);
	case RT_PARAMS_EDIT_TRANS:
	    /* translate solid */
	    m->es_metaball_pnt = (struct wdb_metaball_pnt *)NULL; /* Reset es_metaball_pnt */
	    edit_stra(s);
	    break;
	case RT_PARAMS_EDIT_ROT:
	    /* rot solid about vertex */
	    m->es_metaball_pnt = (struct wdb_metaball_pnt *)NULL; /* Reset es_metaball_pnt */
	    edit_srot(s);
	    break;
	case ECMD_METABALL_PT_PICK:
	    ecmd_metaball_pt_pick(s);
	    break;
	case ECMD_METABALL_PT_MOV:
	    ecmd_metaball_pt_mov(s);
	    break;
	case ECMD_METABALL_PT_DEL:
	    ecmd_metaball_pt_del(s);
	    break;
	case ECMD_METABALL_PT_ADD:
	    ecmd_metaball_pt_add(s);
	    break;
	default:
	    return rt_edit_metaball_pscale(s);
    }

    return 0;
}

int
rt_edit_metaball_edit_xy(
	struct rt_edit *s,
	const vect_t mousevec
	)
{
    vect_t pos_view = VINIT_ZERO;       /* Unrotated view space pos */
    vect_t temp = VINIT_ZERO;
    struct rt_db_internal *ip = &s->es_int;
    bu_clbk_t f = NULL;
    void *d = NULL;

    switch (s->edit_flag) {
	case RT_PARAMS_EDIT_SCALE:
	case ECMD_METABALL_PT_NEXT:
	case ECMD_METABALL_PT_PREV:
	case ECMD_METABALL_PT_DEL:
	case ECMD_METABALL_PT_FLDSTR:
	case ECMD_METABALL_PT_SET_GOO:
	case ECMD_METABALL_RMET:
	case ECMD_METABALL_SET_METHOD:
	case ECMD_METABALL_SET_THRESHOLD:
	    edit_sscale_xy(s, mousevec);
	    return 0;
	case RT_PARAMS_EDIT_TRANS:
	    edit_stra_xy(&pos_view, s, mousevec);
	    break;
	case ECMD_METABALL_PT_PICK:
	case ECMD_METABALL_PT_MOV:
	case ECMD_METABALL_PT_ADD:
	    MAT4X3PNT(pos_view, s->vp->gv_model2view, s->curr_e_axes_pos);
	    pos_view[X] = mousevec[X];
	    pos_view[Y] = mousevec[Y];
	    MAT4X3PNT(temp, s->vp->gv_view2model, pos_view);
	    MAT4X3PNT(s->e_mparam, s->e_invmat, temp);
	    s->e_mvalid = 1;
	    break;
        case RT_PARAMS_EDIT_ROT:
            bu_vls_printf(s->log_str, "RT_PARAMS_EDIT_ROT XY editing setup unimplemented in %s_edit_xy callback\n", EDOBJ[ip->idb_type].ft_label);
            rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
            if (f)
                (*f)(0, NULL, d, NULL);
            return BRLCAD_ERROR;
	default:
	    bu_vls_printf(s->log_str, "%s: XY edit undefined in solid edit mode %d\n", EDOBJ[ip->idb_type].ft_label, s->edit_flag);
	    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	    if (f)
		(*f)(0, NULL, d, NULL);
	    return BRLCAD_ERROR;
    }

    edit_abs_tra(s, pos_view);

    return 0;
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
