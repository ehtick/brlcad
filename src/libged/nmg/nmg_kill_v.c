/*                         N M G _ K I L L _ V. C
 * BRL-CAD
 *
 * Copyright (c) 2015-2025 United States Government as represented by
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
/** @file libged/nmg_kill_v.c
 *
 * The kill V subcommand for nmg top-level command.
 *
 */

#include "common.h"

#include <string.h>

#include "bu/cmd.h"
#include "rt/geom.h"

#include "../ged_private.h"

void remove_vertex(const struct model* m, point_t rv)
{
    struct nmgregion *r;
    struct shell *s;
    struct faceuse *fu;
    struct face *f;
    struct loopuse *lu;
    struct loop *l;
    struct edgeuse *eu;
    struct edge *e;
    struct vertexuse *vu;
    struct vertex *v;

    NMG_CK_MODEL(m);

    /* Traverse NMG model and remove instances of vertexuses.
     * In addition to vertex being removed, associated faceuses, loopuses
     * and edgeuses need to be removed that contained the deleted vertexuse.
     */

    for (BU_LIST_FOR(r, nmgregion, &m->r_hd)) {
	NMG_CK_REGION(r);

	if (r->ra_p) {
	    NMG_CK_REGION_A(r->ra_p);
	}

	for (BU_LIST_FOR(s, shell, &r->s_hd)) {
	    NMG_CK_SHELL(s);

	    if (s->sa_p) {
		NMG_CK_SHELL_A(s->sa_p);
	    }

	    /* Faces in shell */
	    for (BU_LIST_FOR(fu, faceuse, &s->fu_hd)) {
		NMG_CK_FACEUSE(fu);
		f = fu->f_p;
		NMG_CK_FACE(f);

		if (f->g.magic_p) switch (*f->g.magic_p) {
		    case NMG_FACE_G_PLANE_MAGIC:
			break;
		    case NMG_FACE_G_SNURB_MAGIC:
			break;
		}

		/* Loops in face */
		for (BU_LIST_FOR(lu, loopuse, &fu->lu_hd)) {
		    NMG_CK_LOOPUSE(lu);
		    l = lu->l_p;
		    NMG_CK_LOOP(l);

		    if (l->la_p) {
			NMG_CK_LOOP_A(l->la_p);
		    }

		    if (BU_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC) {
			/* Loop of Lone vertex */
			vu = BU_LIST_FIRST(vertexuse, &lu->down_hd);

			/* check and remove vertexuse */
			NMG_CK_VERTEXUSE(vu);
			v = vu->v_p;
			NMG_CK_VERTEX(v);

			if (v->vg_p) {
			    NMG_CK_VERTEX_G(v->vg_p);

			    if ( VNEAR_EQUAL(v->vg_p->coord, rv, BN_TOL_DIST) ) {
				nmg_kvu(vu);
				nmg_klu(lu);
			    }
			}

			continue;
		    }

		    for (BU_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
			NMG_CK_EDGEUSE(eu);
			e = eu->e_p;
			NMG_CK_EDGE(e);

			if (eu->g.magic_p) {
			    switch (*eu->g.magic_p) {
			    case NMG_EDGE_G_LSEG_MAGIC:
				break;
			    case NMG_EDGE_G_CNURB_MAGIC:
				break;
			    }
			}

			vu = eu->vu_p;

			/* check and remove vertexuse */
			NMG_CK_VERTEXUSE(vu);
			v = vu->v_p;
			NMG_CK_VERTEX(v);

			if (v->vg_p) {
			    NMG_CK_VERTEX_G(v->vg_p);

			    if ( VNEAR_EQUAL(v->vg_p->coord,
				 rv, BN_TOL_DIST) ) {
				nmg_kvu(vu);
				nmg_keu(eu);
				nmg_klu(lu);
			    }
			}
		    }
		}
	    }

	    /* Wire loops in shell */
	    for (BU_LIST_FOR(lu, loopuse, &s->lu_hd)) {
		NMG_CK_LOOPUSE(lu);
		l = lu->l_p;
		NMG_CK_LOOP(l);

		if (l->la_p) {
		    NMG_CK_LOOP_A(l->la_p);
		}

		if (BU_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC) {
		    /* Wire loop of Lone vertex */
		    vu = BU_LIST_FIRST(vertexuse, &lu->down_hd);
		    /* check and remove vertexuse */
		    NMG_CK_VERTEXUSE(vu);
		    v = vu->v_p;
		    NMG_CK_VERTEX(v);
		    if (v->vg_p) {
			NMG_CK_VERTEX_G(v->vg_p);
			if ( VNEAR_EQUAL(v->vg_p->coord, rv, BN_TOL_DIST) ) {
			    nmg_kvu(vu);
			    nmg_klu(lu);
			}
		    }
		    continue;
		}

		for (BU_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
		    NMG_CK_EDGEUSE(eu);
		    e = eu->e_p;
		    NMG_CK_EDGE(e);

		    if (eu->g.magic_p) switch (*eu->g.magic_p) {
			case NMG_EDGE_G_LSEG_MAGIC:
			break;
			case NMG_EDGE_G_CNURB_MAGIC:
			break;
		    }
		    vu = eu->vu_p;

		    /* check and remove vertexuse */
		    NMG_CK_VERTEXUSE(vu);
		    v = vu->v_p;
		    NMG_CK_VERTEX(v);

		    if (v->vg_p) {
			NMG_CK_VERTEX_G(v->vg_p);
			if ( VNEAR_EQUAL(v->vg_p->coord, rv, BN_TOL_DIST) ) {
			    nmg_kvu(vu);
			    nmg_keu(eu);
			    nmg_klu(lu);
			}
		    }
		}
	    }

	    /* Wire edges in shell */
	    for (BU_LIST_FOR(eu, edgeuse, &s->eu_hd)) {
		NMG_CK_EDGEUSE(eu);
		e = eu->e_p;
		NMG_CK_EDGE(e);

		if (eu->g.magic_p) {
		    switch (*eu->g.magic_p) {
		    case NMG_EDGE_G_LSEG_MAGIC:
			break;
		    case NMG_EDGE_G_CNURB_MAGIC:
			break;
		    }
		}

		vu = eu->vu_p;

		/* check and remove vertexuse */
		NMG_CK_VERTEXUSE(vu);
		v = vu->v_p;
		NMG_CK_VERTEX(v);

		if (v->vg_p) {
		    NMG_CK_VERTEX_G(v->vg_p);

		    if ( VNEAR_EQUAL(v->vg_p->coord, rv, BN_TOL_DIST) ) {
			nmg_kvu(vu);
			nmg_keu(eu);
		    }
		}
	    }

	    /* Lone vertex in shell */
	    vu = s->vu_p;

	    if (vu) {
		/* check and remove vertexuse */
		NMG_CK_VERTEXUSE(vu);
		v = vu->v_p;
		NMG_CK_VERTEX(v);

		if (v->vg_p) {
		    NMG_CK_VERTEX_G(v->vg_p);

		    if ( VNEAR_EQUAL(v->vg_p->coord, rv, BN_TOL_DIST) ) {
			nmg_kvu(vu);
		    }
		}
	    }
	}
    }
}

int
ged_nmg_kill_v_core(struct ged* gedp, int argc, const char* argv[])
{
    struct rt_db_internal internal;
    struct directory *dp;
    struct model* m;
    const char* name;
    point_t vt;

    static const char *usage = "kill V x y z";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_DRAWABLE(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc < 6) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    /* attempt to resolve and verify */
    name = argv[0];

    dp = db_lookup(gedp->dbip, name, LOOKUP_QUIET);
    if (dp == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "%s does not exist\n", name);
	return BRLCAD_ERROR;
    }

    if (rt_db_get_internal(&internal, dp, gedp->dbip, bn_mat_identity, &rt_uniresource) < 0) {
	bu_vls_printf(gedp->ged_result_str, "rt_db_get_internal() error\n");
	return BRLCAD_ERROR;
    }

    if (internal.idb_type != ID_NMG) {
	bu_vls_printf(gedp->ged_result_str, "%s is not an NMG solid\n", name);
	rt_db_free_internal(&internal);
	return BRLCAD_ERROR;
    }

    vt[0] = atof(argv[3]); vt[1] = atof(argv[4]); vt[2] = atof(argv[5]);

    m = (struct model *)internal.idb_ptr;
    NMG_CK_MODEL(m);

    remove_vertex(m, vt);

    struct rt_wdb *wdbp = wdb_dbopen(gedp->dbip, RT_WDB_TYPE_DB_DEFAULT);
    if (wdb_put_internal(wdbp, name, &internal, 1.0) < 0 ) {
	bu_vls_printf(gedp->ged_result_str, "wdb_put_internal(%s)", argv[1]);
	rt_db_free_internal(&internal);
	return BRLCAD_ERROR;
    }

    rt_db_free_internal(&internal);

    return BRLCAD_OK;
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
