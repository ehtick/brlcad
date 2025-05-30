/*                         E D B O T . C
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
/** @file primitives/edbot.c
 *
 */

#include "common.h"

#include <math.h>
#include <string.h>

#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "rt/geom.h"
#include "rt/primitives/bot.h"
#include "wdb.h"

#include "../edit_private.h"

#define ECMD_BOT_PICKV		30061	/* pick a BOT vertex */
#define ECMD_BOT_PICKE		30062	/* pick a BOT edge */
#define ECMD_BOT_PICKT		30063	/* pick a BOT triangle */
#define ECMD_BOT_MOVEV		30064	/* move a BOT vertex */
#define ECMD_BOT_MOVEE		30065	/* move a BOT edge */
#define ECMD_BOT_MOVET		30066	/* move a BOT triangle */
#define ECMD_BOT_MODE		30067	/* set BOT mode */
#define ECMD_BOT_ORIENT		30068	/* set BOT face orientation */
#define ECMD_BOT_THICK		30069	/* set face thickness (one or all) */
#define ECMD_BOT_FMODE		30070	/* set face mode (one or all) */
#define ECMD_BOT_FDEL		30071	/* delete current face */
#define ECMD_BOT_FLAGS		30072	/* set BOT flags */

void *
rt_edit_bot_prim_edit_create(struct rt_edit *UNUSED(s))
{
    struct rt_bot_edit *b;
    BU_GET(b, struct rt_bot_edit);

    b->bot_verts[0] = -1;
    b->bot_verts[1] = -1;
    b->bot_verts[2] = -1;

    return (void *)b;
}

void
rt_edit_bot_prim_edit_destroy(struct rt_bot_edit *b)
{
    if (!b)
	return;

    b->bot_verts[0] = -1;
    b->bot_verts[1] = -1;
    b->bot_verts[2] = -1;

    BU_PUT(b, struct rt_bot_edit);
}

void
rt_edit_bot_prim_edit_reset(struct rt_edit *s)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    b->bot_verts[0] = -1;
    b->bot_verts[1] = -1;
    b->bot_verts[2] = -1;
}

void
rt_edit_bot_set_edit_mode(struct rt_edit *s, int mode)
{
    rt_edit_set_edflag(s, mode);

    switch (mode) {
	case ECMD_BOT_MOVEV:
	case ECMD_BOT_MOVEE:
	case ECMD_BOT_MOVET:
	    s->edit_mode = RT_PARAMS_EDIT_TRANS;
	    break;
	case ECMD_BOT_PICKV:
	case ECMD_BOT_PICKE:
	case ECMD_BOT_PICKT:
	    s->edit_mode = RT_PARAMS_EDIT_PICK;
	    break;
	default:
	    break;
    };

    bu_clbk_t f = NULL;
    void *d = NULL;
    int flag = 1;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_EAXES_POS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, &flag);
}

static void
bot_ed(struct rt_edit *s, int arg, int UNUSED(a), int UNUSED(b), void *UNUSED(data))
{
    rt_edit_set_edflag(s, arg);

    switch (arg) {
	case ECMD_BOT_MOVEV:
	case ECMD_BOT_MOVEE:
	case ECMD_BOT_MOVET:
	    s->edit_mode = RT_PARAMS_EDIT_TRANS;
	    break;
	case ECMD_BOT_PICKV:
	case ECMD_BOT_PICKE:
	case ECMD_BOT_PICKT:
	    s->edit_mode = RT_PARAMS_EDIT_PICK;
	    break;
	default:
	    break;
    };

    // TODO - should we be calling this here?
    rt_edit_process(s);

    bu_clbk_t f = NULL;
    void *d = NULL;
    int flag = 1;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_EAXES_POS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, &flag);
}
struct rt_edit_menu_item bot_menu[] = {
    { "BOT MENU", NULL, 0 },
    { "Pick Vertex", bot_ed, ECMD_BOT_PICKV },
    { "Pick Edge", bot_ed, ECMD_BOT_PICKE },
    { "Pick Triangle", bot_ed, ECMD_BOT_PICKT },
    { "Move Vertex", bot_ed, ECMD_BOT_MOVEV },
    { "Move Edge", bot_ed, ECMD_BOT_MOVEE },
    { "Move Triangle", bot_ed, ECMD_BOT_MOVET },
    { "Delete Triangle", bot_ed, ECMD_BOT_FDEL },
    { "Select Mode", bot_ed, ECMD_BOT_MODE },
    { "Select Orientation", bot_ed, ECMD_BOT_ORIENT },
    { "Set flags", bot_ed, ECMD_BOT_FLAGS },
    { "Set Face Thickness", bot_ed, ECMD_BOT_THICK },
    { "Set Face Mode", bot_ed, ECMD_BOT_FMODE },
    { "", NULL, 0 }
};

struct rt_edit_menu_item *
rt_edit_bot_menu_item(const struct bn_tol *UNUSED(tol))
{
    return bot_menu;
}

void
rt_edit_bot_labels(
	int *num_lines,
	point_t *lines,
	struct rt_point_labels *pl,
	int UNUSED(max_pl),
	const mat_t xform,
	struct rt_edit *s,
	struct bn_tol *UNUSED(tol))
{
    struct rt_db_internal *ip = &s->es_int;
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    point_t pos_view;
    int npl = 0;

#define POINT_LABEL_STR(_pt, _str) { \
    VMOVE(pl[npl].pt, _pt); \
    bu_strlcpy(pl[npl++].str, _str, sizeof(pl[0].str)); }

    struct rt_bot_internal *bot = (struct rt_bot_internal *)ip->idb_ptr;

    // Conditional labeling
    if (b->bot_verts[2] > -1 &&
	    b->bot_verts[1] > -1 &&
	    b->bot_verts[0] > -1)
    {

	RT_BOT_CK_MAGIC(bot);

	/* editing a face */
	point_t mid_pt;
	point_t p1, p2, p3;
	fastf_t one_third = 1.0/3.0;

	MAT4X3PNT(p1, xform, &bot->vertices[b->bot_verts[0]*3]);
	MAT4X3PNT(p2, xform, &bot->vertices[b->bot_verts[1]*3]);
	MAT4X3PNT(p3, xform, &bot->vertices[b->bot_verts[2]*3]);
	VADD3(mid_pt, p1, p2, p3);

	VSCALE(mid_pt, mid_pt, one_third);

	*num_lines = 3;
	VMOVE(lines[0], mid_pt);
	VMOVE(lines[1], p1);
	VMOVE(lines[2], mid_pt);
	VMOVE(lines[3], p2);
	VMOVE(lines[4], mid_pt);
	VMOVE(lines[5], p3);

    } else if (b->bot_verts[1] > -1 && b->bot_verts[0] > -1) {

	RT_BOT_CK_MAGIC(bot);

	/* editing an edge */
	point_t mid_pt;

	VBLEND2(mid_pt, 0.5, &bot->vertices[b->bot_verts[0]*3],
		0.5, &bot->vertices[b->bot_verts[1]*3]);

	MAT4X3PNT(pos_view, xform, mid_pt);
	POINT_LABEL_STR(pos_view, "edge");

    }
    if (b->bot_verts[0] > -1) {

	RT_BOT_CK_MAGIC(bot);

	/* editing something, always label the vertex (this is the keypoint) */
	MAT4X3PNT(pos_view, xform, &bot->vertices[b->bot_verts[0]*3]);
	POINT_LABEL_STR(pos_view, "pt");
    }

    pl[npl].str[0] = '\0';	/* Mark ending */

}

const char *
rt_edit_bot_keypoint(
	point_t *pt,
	const char *keystr,
	const mat_t mat,
	struct rt_edit *s,
	const struct bn_tol *tol)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    struct rt_db_internal *ip = &s->es_int;
    const char *strp = OBJ[ip->idb_type].ft_keypoint(pt, keystr, mat, ip, tol);
    // If we're editing, use that position instead
    if (b->bot_verts[0] > -1) {
	point_t mpt = VINIT_ZERO;
	struct rt_bot_internal *bot = (struct rt_bot_internal *)ip->idb_ptr;
	RT_BOT_CK_MAGIC(bot);
	VMOVE(mpt, &bot->vertices[b->bot_verts[0]*3]);
	MAT4X3PNT(*pt, mat, mpt);
    }
    return strp;
}

void
ecmd_bot_mode(struct rt_edit *s)
{
    struct rt_bot_internal *bot = (struct rt_bot_internal *)s->es_int.idb_ptr;

    RT_BOT_CK_MAGIC(bot);
    int old_mode = bot->mode;

    // Set bot->mode using the callback
    bu_clbk_t f = NULL;
    void *d = NULL;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_BOT_MODE, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, s);

    if (bot->mode == RT_BOT_PLATE || bot->mode == RT_BOT_PLATE_NOCOS) {
	if (old_mode != RT_BOT_PLATE && old_mode != RT_BOT_PLATE_NOCOS) {
	    /* need to create some thicknesses */
	    bot->thickness = (fastf_t *)bu_calloc(bot->num_faces, sizeof(fastf_t), "BOT thickness");
	    bot->face_mode = bu_bitv_new(bot->num_faces);
	}
    } else {
	if (old_mode == RT_BOT_PLATE || old_mode == RT_BOT_PLATE_NOCOS) {
	    /* free the per face memory */
	    bu_free((char *)bot->thickness, "BOT thickness");
	    bot->thickness = (fastf_t *)NULL;
	    bu_free((char *)bot->face_mode, "BOT face_mode");
	    bot->face_mode = (struct bu_bitv *)NULL;
	}
    }
}

void
ecmd_bot_orient(struct rt_edit *s)
{
    // Set bot->orientation using the callback
    bu_clbk_t f = NULL;
    void *d = NULL;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_BOT_ORIENT, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, s);
}

int
ecmd_bot_thick(struct rt_edit *s)
{

    if (s->e_inpara != 1) {
	bu_vls_printf(s->log_str, "ERROR: only one argument needed\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    if (s->e_para[0] <= 0.0) {
	bu_vls_printf(s->log_str, "ERROR: SCALE FACTOR <= 0\n");
	s->e_inpara = 0;
	return BRLCAD_ERROR;
    }

    /* must convert to base units */
    s->e_para[0] *= s->local2base;

    // Set bot->thickness array using the callback
    bu_clbk_t f = NULL;
    void *d = NULL;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_BOT_THICK, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, s);

    return BRLCAD_OK;
}

void
ecmd_bot_flags(struct rt_edit *s)
{
    // Set bot->thickness array using the callback
    bu_clbk_t f = NULL;
    void *d = NULL;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_BOT_FLAGS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, s);
}

void
ecmd_bot_fmode(struct rt_edit *s)
{
    // Set bot->face_mode using the callback
    bu_clbk_t f = NULL;
    void *d = NULL;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_BOT_FMODE, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, s);
}

int
ecmd_bot_fdel(struct rt_edit *s)
{
    struct rt_bot_internal *bot =
	(struct rt_bot_internal *)s->es_int.idb_ptr;
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;

    int j, face_no;

    RT_BOT_CK_MAGIC(bot);

    if (b->bot_verts[0] < 0 || b->bot_verts[1] < 0 || b->bot_verts[2] < 0) {
	bu_log("No Face selected!\n");
	return BRLCAD_ERROR;
    }

    face_no = -1;
    for (size_t i=0; i < bot->num_faces; i++) {
	if (b->bot_verts[0] == bot->faces[i*3] &&
		b->bot_verts[1] == bot->faces[i*3+1] &&
		b->bot_verts[2] == bot->faces[i*3+2])
	{
	    face_no = i;
	    break;
	}
    }
    if (face_no < 0) {
	bu_log("Cannot find selected face!\n");
	return BRLCAD_ERROR;
    }
    bot->num_faces--;
    for (size_t i=face_no; i<bot->num_faces; i++) {
	j = i + 1;
	bot->faces[3*i] = bot->faces[3*j];
	bot->faces[3*i + 1] = bot->faces[3*j + 1];
	bot->faces[3*i + 2] = bot->faces[3*j + 2];
	if (bot->thickness)
	    bot->thickness[i] = bot->thickness[j];
    }

    if (bot->face_mode) {
	struct bu_bitv *new_bitv;

	new_bitv = bu_bitv_new(bot->num_faces);
	for (size_t i=0; i<(size_t)face_no; i++) {
	    if (BU_BITTEST(bot->face_mode, i))
		BU_BITSET(new_bitv, i);
	}
	for (size_t i=face_no; i<bot->num_faces; i++) {
	    j = i+1;
	    if (BU_BITTEST(bot->face_mode, j))
		BU_BITSET(new_bitv, i);
	}
	bu_bitv_free(bot->face_mode);
	bot->face_mode = new_bitv;
    }
    b->bot_verts[0] = -1;
    b->bot_verts[1] = -1;
    b->bot_verts[2] = -1;

    return BRLCAD_OK;
}

void
ecmd_bot_movev(struct rt_edit *s)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    struct rt_bot_internal *bot = (struct rt_bot_internal *)s->es_int.idb_ptr;
    int vert;
    point_t new_pt = VINIT_ZERO;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_BOT_CK_MAGIC(bot);

    if (b->bot_verts[0] < 0) {
	bu_log("No BOT point selected\n");
	return;
    }

    if (b->bot_verts[1] >= 0 && b->bot_verts[2] >= 0) {
	bu_log("A triangle is selected, not a BOT point!\n");
	return;
    }

    if (b->bot_verts[1] >= 0) {
	bu_log("An edge is selected, not a BOT point!\n");
	return;
    }

    vert = b->bot_verts[0];
    if (s->e_mvalid) {
	VMOVE(new_pt, s->e_mparam);
    } else if (s->e_inpara == 3) {
	/* must convert to base units */
	s->e_para[0] *= s->local2base;
	s->e_para[1] *= s->local2base;
	s->e_para[2] *= s->local2base;

	if (s->mv_context) {
	    /* apply s->e_invmat to convert to real model space */
	    MAT4X3PNT(new_pt, s->e_invmat, s->e_para);
	} else {
	    VMOVE(new_pt, s->e_para);
	}
    } else if (s->e_inpara && s->e_inpara != 3) {
	bu_vls_printf(s->log_str, "x y z coordinates required for point movement\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return;
    } else if (!s->e_mvalid && !s->e_inpara) {
	return;
    }

    VMOVE(&bot->vertices[vert*3], new_pt);
}

void
ecmd_bot_movee(struct rt_edit *s)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    struct rt_bot_internal *bot = (struct rt_bot_internal *)s->es_int.idb_ptr;
    int v1, v2;
    vect_t diff;
    point_t new_pt = VINIT_ZERO;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_BOT_CK_MAGIC(bot);

    if (b->bot_verts[0] < 0 || b->bot_verts[1] < 0) {
	bu_vls_printf(s->log_str, "No BOT edge selected\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return;
    }

    if (b->bot_verts[2] >= 0) {
	bu_log("A triangle is selected, not a BOT edge!\n");
	return;
    }
    v1 = b->bot_verts[0];
    v2 = b->bot_verts[1];
    if (s->e_mvalid) {
	VMOVE(new_pt, s->e_mparam);
    } else if (s->e_inpara == 3) {
	/* must convert to base units */
	s->e_para[0] *= s->local2base;
	s->e_para[1] *= s->local2base;
	s->e_para[2] *= s->local2base;

	if (s->mv_context) {
	    /* apply s->e_invmat to convert to real model space */
	    MAT4X3PNT(new_pt, s->e_invmat, s->e_para);
	} else {
	    VMOVE(new_pt, s->e_para);
	}
    } else if (s->e_inpara && s->e_inpara != 3) {
	bu_vls_printf(s->log_str, "x y z coordinates required for point movement\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return;
    } else if (!s->e_mvalid && !s->e_inpara) {
	return;
    }


    VSUB2(diff, new_pt, &bot->vertices[v1*3]);
    VMOVE(&bot->vertices[v1*3], new_pt);
    VADD2(&bot->vertices[v2*3], &bot->vertices[v2*3], diff);
}

void
ecmd_bot_movet(struct rt_edit *s)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    struct rt_bot_internal *bot = (struct rt_bot_internal *)s->es_int.idb_ptr;
    int v1, v2, v3;
    point_t new_pt = VINIT_ZERO;
    vect_t diff;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_BOT_CK_MAGIC(bot);

    if (b->bot_verts[0] < 0 || b->bot_verts[1] < 0 || b->bot_verts[2] < 0) {
	bu_vls_printf(s->log_str, "No BOT triangle selected\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return;
    }
    v1 = b->bot_verts[0];
    v2 = b->bot_verts[1];
    v3 = b->bot_verts[2];

    if (s->e_mvalid) {
	VMOVE(new_pt, s->e_mparam);
    } else if (s->e_inpara == 3) {
	/* must convert to base units */
	s->e_para[0] *= s->local2base;
	s->e_para[1] *= s->local2base;
	s->e_para[2] *= s->local2base;

	if (s->mv_context) {
	    /* apply s->e_invmat to convert to real model space */
	    MAT4X3PNT(new_pt, s->e_invmat, s->e_para);
	} else {
	    VMOVE(new_pt, s->e_para);
	}
    } else if (s->e_inpara && s->e_inpara != 3) {
	bu_vls_printf(s->log_str, "x y z coordinates required for point movement\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return;
    } else if (!s->e_mvalid && !s->e_inpara) {
	return;
    }

    VSUB2(diff, new_pt, &bot->vertices[v1*3]);
    VMOVE(&bot->vertices[v1*3], new_pt);
    VADD2(&bot->vertices[v2*3], &bot->vertices[v2*3], diff);
    VADD2(&bot->vertices[v3*3], &bot->vertices[v3*3], diff);
}

int
ecmd_bot_pickv(struct rt_edit *s, const vect_t mousevec)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    vect_t pos_view = VINIT_ZERO;	/* Unrotated view space pos */
    struct rt_bot_internal *bot = (struct rt_bot_internal *)s->es_int.idb_ptr;
    int tmp_vert;
    char tmp_msg[256];
    point_t selected_pt;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_BOT_CK_MAGIC(bot);

    MAT4X3PNT(pos_view, s->vp->gv_model2view, s->curr_e_axes_pos);
    pos_view[X] = mousevec[X];
    pos_view[Y] = mousevec[Y];

    tmp_vert = rt_bot_find_v_nearest_pt2(bot, pos_view, s->vp->gv_model2view);
    if (tmp_vert < 0) {
	bu_vls_printf(s->log_str, "ECMD_BOT_PICKV: unable to find a vertex!\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return BRLCAD_ERROR;
    }

    b->bot_verts[0] = tmp_vert;
    b->bot_verts[1] = -1;
    b->bot_verts[2] = -1;
    VSCALE(selected_pt, &bot->vertices[tmp_vert*3], s->base2local);
    sprintf(tmp_msg, "picked point at (%g %g %g), vertex #%d\n", V3ARGS(selected_pt), tmp_vert);
    bu_vls_printf(s->log_str, "%s", tmp_msg);
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, NULL);

    return BRLCAD_OK;
}

int
ecmd_bot_picke(struct rt_edit *s, const vect_t mousevec)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    vect_t pos_view = VINIT_ZERO;	/* Unrotated view space pos */
    struct rt_bot_internal *bot = (struct rt_bot_internal *)s->es_int.idb_ptr;
    int vert1, vert2;
    char tmp_msg[256];
    point_t from_pt, to_pt;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_BOT_CK_MAGIC(bot);

    MAT4X3PNT(pos_view, s->vp->gv_model2view, s->curr_e_axes_pos);
    pos_view[X] = mousevec[X];
    pos_view[Y] = mousevec[Y];

    if (rt_bot_find_e_nearest_pt2(&vert1, &vert2, bot, pos_view, s->vp->gv_model2view)) {
	bu_vls_printf(s->log_str, "ECMD_BOT_PICKE: unable to find an edge!\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return BRLCAD_ERROR;
    }

    b->bot_verts[0] = vert1;
    b->bot_verts[1] = vert2;
    b->bot_verts[2] = -1;
    VSCALE(from_pt, &bot->vertices[vert1*3], s->base2local);
    VSCALE(to_pt, &bot->vertices[vert2*3], s->base2local);
    sprintf(tmp_msg, "picked edge from (%g %g %g) to (%g %g %g)\n", V3ARGS(from_pt), V3ARGS(to_pt));
    bu_vls_printf(s->log_str, "%s", tmp_msg);
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, NULL);

    return BRLCAD_OK;
}

void
ecmd_bot_pickt(struct rt_edit *s, const vect_t mousevec)
{
    struct rt_bot_internal *bot = (struct rt_bot_internal *)s->es_int.idb_ptr;
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    point_t start_pt, tmp;
    vect_t dir;
    size_t i;
    int hits;
    int v1, v2, v3;
    point_t pt1, pt2, pt3;
    struct bu_vls vls = BU_VLS_INIT_ZERO;

    RT_BOT_CK_MAGIC(bot);

    VSET(tmp, mousevec[X], mousevec[Y], 0.0);
    MAT4X3PNT(start_pt, s->vp->gv_view2model, tmp);
    VSET(tmp, 0, 0, 1);
    MAT4X3VEC(dir, s->vp->gv_view2model, tmp);

    bu_vls_strcat(&vls, " {");
    hits = 0;
    for (i=0; i<bot->num_faces; i++) {
	v1 = bot->faces[i*3];
	v2 = bot->faces[i*3+1];
	v3 = bot->faces[i*3+2];
	VMOVE(pt1, &bot->vertices[v1*3]);
	VMOVE(pt2, &bot->vertices[v2*3]);
	VMOVE(pt3, &bot->vertices[v3*3]);

	if (bg_does_ray_isect_tri(start_pt, dir, pt1, pt2, pt3, tmp)) {
	    hits++;
	    bu_vls_printf(&vls, " { %d %d %d }", v1, v2, v3);
	}
    }
    bu_vls_strcat(&vls, " } ");

    if (hits == 0) {
	b->bot_verts[0] = -1;
	b->bot_verts[1] = -1;
	b->bot_verts[2] = -1;
	bu_vls_free(&vls);
    }
    if (hits == 1) {
	sscanf(bu_vls_cstr(&vls), " { { %d %d %d", &b->bot_verts[0], &b->bot_verts[1], &b->bot_verts[2]);
	bu_vls_free(&vls);
    } else {
	// Set bot->mode using the callback
	void *usr_bk = s->u_ptr;
	s->u_ptr = &vls;

	bu_clbk_t f = NULL;
	void *d = NULL;
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_BOT_PICKT, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, s);

	s->u_ptr = usr_bk;
	bu_vls_free(&vls);
    }
}

int
rt_edit_bot_edit(struct rt_edit *s)
{
    struct rt_bot_edit *b = (struct rt_bot_edit *)s->ipe_ptr;
    switch (s->edit_flag) {
	case RT_PARAMS_EDIT_SCALE:
	    /* scale the solid uniformly about its vertex point */
	    b->bot_verts[0] = -1;
	    b->bot_verts[1] = -1;
	    b->bot_verts[2] = -1;
	    return edit_sscale(s);
	case RT_PARAMS_EDIT_TRANS:
	    /* translate solid */
	    b->bot_verts[0] = -1;
	    b->bot_verts[1] = -1;
	    b->bot_verts[2] = -1;
	    edit_stra(s);
	    break;
	case RT_PARAMS_EDIT_ROT:
	    /* rot solid about vertex */
	    b->bot_verts[0] = -1;
	    b->bot_verts[1] = -1;
	    b->bot_verts[2] = -1;
	    edit_srot(s);
	    break;
	case ECMD_BOT_MODE:
	    ecmd_bot_mode(s);
	    break;
	case ECMD_BOT_ORIENT:
	    ecmd_bot_orient(s);
	    break;
	case ECMD_BOT_THICK:
	    return ecmd_bot_thick(s);
	case ECMD_BOT_FLAGS:
	    ecmd_bot_flags(s);
	    break;
	case ECMD_BOT_FMODE:
	    ecmd_bot_fmode(s);
	    break;
	case ECMD_BOT_FDEL:
	    if (ecmd_bot_fdel(s) != BRLCAD_OK)
		return -1;
	    break;
	case ECMD_BOT_MOVEV:
	    ecmd_bot_movev(s);
	    break;
	case ECMD_BOT_MOVEE:
	    ecmd_bot_movee(s);
	    break;
	case ECMD_BOT_MOVET:
	    ecmd_bot_movet(s);
	    break;
	case ECMD_BOT_PICKV:
	case ECMD_BOT_PICKE:
	case ECMD_BOT_PICKT:
	    break;
    }

    return 0;
}

int
rt_edit_bot_edit_xy(
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
	case ECMD_BOT_MODE :
	case ECMD_BOT_ORIENT:
	case ECMD_BOT_THICK:
	case ECMD_BOT_FMODE:
	case ECMD_BOT_FDEL :
	case ECMD_BOT_FLAGS:
	    edit_sscale_xy(s, mousevec);
	    return 0;
	case RT_PARAMS_EDIT_TRANS:
	    edit_stra_xy(&pos_view, s, mousevec);
	    break;
	case ECMD_BOT_PICKV:
	    if (ecmd_bot_pickv(s, mousevec) != BRLCAD_OK)
		return BRLCAD_ERROR;
	    break;
	case ECMD_BOT_PICKE:
	    if (ecmd_bot_picke(s, mousevec) != BRLCAD_OK)
		return BRLCAD_ERROR;
	    break;
	case ECMD_BOT_PICKT:
	    ecmd_bot_pickt(s, mousevec);
	    break;
	case ECMD_BOT_MOVEV:
	case ECMD_BOT_MOVEE:
	case ECMD_BOT_MOVET:
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
