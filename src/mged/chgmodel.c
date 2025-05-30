/*                      C H G M O D E L . C
 * BRL-CAD
 *
 * Copyright (c) 1985-2025 United States Government as represented by
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
/** @file mged/chgmodel.c
 *
 * This module contains functions which change particulars of the
 * model, generally on a single solid or combination.
 * Changes to the tree structure of the model are done in chgtree.c
 *
 */

#include "common.h"

#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "vmath.h"
#include "bu/units.h"
#include "bn.h"
#include "raytrace.h"
#include "nmg.h"
#include "rt/geom.h"
#include "ged.h"
#include "wdb.h"

#include "./mged.h"
#include "./mged_dm.h"
#include "./sedit.h"
#include "./cmd.h"
#include "./f_cmd.h"


/* defined in chgview.c */
extern int edit_com(struct mged_state *s, int argc, const char *argv[]);

/* defined in buttons.c */
extern int be_s_trans(ClientData, Tcl_Interp *, int, char **);

/*
 * Create a new solid of a given type
 * (Generic, or explicit)
 */
int
f_make(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    struct cmdtab *ctp = (struct cmdtab *)clientData;
    MGED_CK_CMD(ctp);
    struct mged_state *s = ctp->s;
    Tcl_DString ds;
    int ret;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc == 3) {
	const char *av[8];
	char center[512];
	char scale[128];

	sprintf(center, "%.17f %.17f %.17f",
		(ZERO(view_state->vs_gvp->gv_center[MDX])) ? 0.0 : -view_state->vs_gvp->gv_center[MDX],
		(ZERO(view_state->vs_gvp->gv_center[MDY])) ? 0.0 : -view_state->vs_gvp->gv_center[MDY],
		(ZERO(view_state->vs_gvp->gv_center[MDZ])) ? 0.0 : -view_state->vs_gvp->gv_center[MDZ]);
	sprintf(scale, "%.17f", view_state->vs_gvp->gv_scale * 2.0);

	av[0] = argv[0];
	av[1] = "-o";
	av[2] = center;
	av[3] = "-s";
	av[4] = scale;
	av[5] = argv[1];
	av[6] = argv[2];
	av[7] = (char *)0;

	ret = ged_exec(s->gedp, 7, (const char **)av);
    } else
	ret = ged_exec(s->gedp, argc, (const char **)argv);

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, bu_vls_addr(s->gedp->ged_result_str), -1);
    Tcl_DStringResult(interp, &ds);

    if (ret == BRLCAD_OK) {
	const char *av[4];

	av[0] = "draw";
	av[1] = "-R";
	av[2] = argv[argc-2];
	av[3] = NULL;
	edit_com(s, 3, av);
    } else {
	return TCL_ERROR;
    }

    return TCL_OK;
}


int
mged_rot_obj(struct mged_state *s, int iflag, fastf_t *argvect)
{
    point_t model_pt;
    point_t point;
    point_t s_point;
    mat_t temp;
    vect_t v_work;

    s->update_views = 1;
    dm_set_dirty(DMP, 1);

    if (movedir != ROTARROW) {
	/* NOT in object rotate mode - put it in obj rot */
	movedir = ROTARROW;
    }

    /* find point for rotation to take place wrt */
    VMOVE(model_pt, s->s_edit->e_keypoint);

    MAT4X3PNT(point, s->s_edit->model_changes, model_pt);

    /* Find absolute translation vector to go from "model_pt" to
     * "point" without any of the rotations in "s->s_edit->model_changes"
     */
    VSCALE(s_point, point, s->s_edit->model_changes[15]);
    VSUB2(v_work, s_point, model_pt);

    /* REDO "s->s_edit->model_changes" such that:
     * 1. NO rotations (identity)
     * 2. trans == v_work
     * 3. same scale factor
     */
    MAT_IDN(temp);
    MAT_DELTAS_VEC(temp, v_work);
    temp[15] = s->s_edit->model_changes[15];
    MAT_COPY(s->s_edit->model_changes, temp);

    /* build new rotation matrix */
    MAT_IDN(temp);
    bn_mat_angles(temp, argvect[0], argvect[1], argvect[2]);

    if (iflag) {
	/* apply accumulated rotations */
	bn_mat_mul2(s->s_edit->acc_rot_sol, temp);
    }

    /*XXX*/ MAT_COPY(s->s_edit->acc_rot_sol, temp); /* used to rotate solid/object axis */

    /* Record the new rotation matrix into the revised
     * s->s_edit->model_changes matrix wrt "point"
     */
    wrt_point(s->s_edit->model_changes, temp, s->s_edit->model_changes, point);

    new_edit_mats(s);

    return TCL_OK;
}


/* allow precise changes to object rotation */
int
f_rot_obj(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    struct cmdtab *ctp = (struct cmdtab *)clientData;
    MGED_CK_CMD(ctp);
    struct mged_state *s = ctp->s;

    int iflag = 0;
    vect_t argvect;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 4 || 5 < argc) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "help %s", argv[0]);
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if (not_state(s, ST_O_EDIT, "Object Rotation"))
	return TCL_ERROR;

    /* Check for -i option */
    if (argv[1][0] == '-' && argv[1][1] == 'i') {
	iflag = 1;  /* treat arguments as incremental values */
	++argv;
	--argc;
    }

    if (argc != 4)
	return TCL_ERROR;

    argvect[0] = atof(argv[1]);
    argvect[1] = atof(argv[2]);
    argvect[2] = atof(argv[3]);

    return mged_rot_obj(s, iflag, argvect);
}


/* allow precise changes to object scaling, both local & global */
int
f_sc_obj(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    struct cmdtab *ctp = (struct cmdtab *)clientData;
    MGED_CK_CMD(ctp);
    struct mged_state *s = ctp->s;

    mat_t incr;
    vect_t point, temp;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 2 || 2 < argc) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "help oscale");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if (not_state(s, ST_O_EDIT, "Object Scaling"))
	return TCL_ERROR;

    if (atof(argv[1]) <= 0.0) {
	Tcl_AppendResult(interp, "ERROR: scale factor <=  0\n", (char *)NULL);
	return TCL_ERROR;
    }

    s->update_views = 1;
    dm_set_dirty(DMP, 1);

    MAT_IDN(incr);

    /* switch depending on type of scaling to do */
    switch (edobj) {
	default:
	case BE_O_SCALE:
	    /* global scaling */
	    incr[15] = 1.0 / (atof(argv[1]) * s->s_edit->model_changes[15]);
	    break;
	case BE_O_XSCALE:
	    /* local scaling ... X-axis */
	    incr[0] = atof(argv[1]) / s->s_edit->acc_sc[0];
	    s->s_edit->acc_sc[0] = atof(argv[1]);
	    break;
	case BE_O_YSCALE:
	    /* local scaling ... Y-axis */
	    incr[5] = atof(argv[1]) / s->s_edit->acc_sc[1];
	    s->s_edit->acc_sc[1] = atof(argv[1]);
	    break;
	case BE_O_ZSCALE:
	    /* local scaling ... Z-axis */
	    incr[10] = atof(argv[1]) / s->s_edit->acc_sc[2];
	    s->s_edit->acc_sc[2] = atof(argv[1]);
	    break;
    }

    /* find point the scaling is to take place wrt */
    VMOVE(temp, s->s_edit->e_keypoint);

    MAT4X3PNT(point, s->s_edit->model_changes, temp);

    wrt_point(s->s_edit->model_changes, incr, s->s_edit->model_changes, point);
    new_edit_mats(s);

    return TCL_OK;
}


/*
 * Bound to command "translate"
 *
 * Allow precise changes to object translation
 */
int
f_tr_obj(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    struct cmdtab *ctp = (struct cmdtab *)clientData;
    MGED_CK_CMD(ctp);
    struct mged_state *s = ctp->s;

    int i;
    mat_t incr, old;
    vect_t model_sol_pt, model_incr, ed_sol_pt, new_vertex;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 4 || 4 < argc) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "help translate");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if (s->global_editing_state == ST_S_EDIT) {
	/* In solid edit mode,
	 * perform the equivalent of "press sxy" and "p xyz"
	 */
	if (be_s_trans(ctp, NULL, 0, NULL) == TCL_ERROR)
	    return TCL_ERROR;
	return f_param(clientData, interp, argc, argv);
    }

    if (not_state(s, ST_O_EDIT, "Object Translation"))
	return TCL_ERROR;

    /* Remainder of code concerns object edit case */

    s->update_views = 1;
    dm_set_dirty(DMP, 1);

    MAT_IDN(incr);
    MAT_IDN(old);

    if ((movedir & (RARROW|UARROW)) == 0) {
	/* put in object trans mode */
	movedir = UARROW | RARROW;
    }

    for (i=0; i<3; i++) {
	new_vertex[i] = atof(argv[i+1]) * s->dbip->dbi_local2base;
    }

    VMOVE(model_sol_pt, s->s_edit->e_keypoint);

    MAT4X3PNT(ed_sol_pt, s->s_edit->model_changes, model_sol_pt);
    VSUB2(model_incr, new_vertex, ed_sol_pt);
    MAT_DELTAS_VEC(incr, model_incr);
    MAT_COPY(old, s->s_edit->model_changes);
    bn_mat_mul(s->s_edit->model_changes, incr, old);
    new_edit_mats(s);

    return TCL_OK;
}


/*
 * Usage: qorot x y z dx dy dz theta
 *
 * rotate an object through a specified angle
 * about a specified ray.
 */
int
f_qorot(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    struct cmdtab *ctp = (struct cmdtab *)clientData;
    MGED_CK_CMD(ctp);
    struct mged_state *s = ctp->s;

    mat_t temp;
    vect_t specified_pt, direc;
    double ang;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 8 || 8 < argc) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "help qorot");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if (not_state(s, ST_O_EDIT, "Object Rotation"))
	return TCL_ERROR;

    if (movedir != ROTARROW) {
	/* NOT in object rotate mode - put it in obj rot */
	movedir = ROTARROW;
    }
    VSET(specified_pt, atof(argv[1]), atof(argv[2]), atof(argv[3]));
    VSCALE(specified_pt, specified_pt, s->dbip->dbi_local2base);
    VSET(direc, atof(argv[4]), atof(argv[5]), atof(argv[6]));

    if (NEAR_ZERO(direc[0], SQRT_SMALL_FASTF) &&
	NEAR_ZERO(direc[1], SQRT_SMALL_FASTF) &&
	NEAR_ZERO(direc[2], SQRT_SMALL_FASTF)) {
	Tcl_AppendResult(interp, "ERROR: magnitude of direction vector >=  0\n", (char *)NULL);
	return TCL_ERROR;
    }
    VUNITIZE(direc);

    ang = atof(argv[7]) * DEG2RAD;

    /* Get matrix for rotation about a point, direction vector and apply to
     * s->s_edit->model_changes matrix
     */
    bn_mat_arb_rot(temp, specified_pt, direc, ang);
    bn_mat_mul2(temp, s->s_edit->model_changes);

    new_edit_mats(s);

    return TCL_OK;
}


void
set_localunit_TclVar(struct mged_state *s)
{
    struct bu_vls vls = BU_VLS_INIT_ZERO;
    struct bu_vls units_vls = BU_VLS_INIT_ZERO;
    const char *str = NULL;

    if (s->dbip == DBI_NULL)
	return;

    str = bu_units_string(s->dbip->dbi_local2base);
    if (str)
	bu_vls_strcpy(&units_vls, str);
    else
	bu_vls_printf(&units_vls, "%gmm", s->dbip->dbi_local2base);

    bu_vls_strcpy(&vls, "localunit");
    Tcl_SetVar(s->interp, bu_vls_addr(&vls), bu_vls_addr(&units_vls), TCL_GLOBAL_ONLY);

    bu_vls_free(&vls);
    bu_vls_free(&units_vls);
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
