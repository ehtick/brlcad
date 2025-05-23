/*                       F A C E D E F . C
 * BRL-CAD
 *
 * Copyright (c) 1986-2025 United States Government as represented by
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
/** @file mged/facedef.c
 *
 */

#include "common.h"

#include <stdlib.h>
#include <math.h>
#include <signal.h>

#include "vmath.h"
#include "rt/geom.h"
#include "raytrace.h"
#include "./mged.h"
#include "./sedit.h"


char *p_rotfb[] = {
    "Enter rot, fb angles: ",
    "Enter fb angle: ",
    "Enter fixed vertex(v#) or point(X Y Z): ",
    "Enter Y, Z of point: ",
    "Enter Z of point: "
};


char *p_3pts[] = {
    "Enter X, Y, Z of point",
    "Enter Y, Z of point",
    "Enter Z of point"
};


char *p_pleqn[] = {
    "Enter A, B, C, D of plane equation: ",
    "Enter B, C, D of plane equation: ",
    "Enter C, D of plane equation: ",
    "Enter D of plane equation: "
};


char *p_nupnt[] = {
    "Enter X, Y, Z of fixed point: ",
    "Enter Y, Z of fixed point: ",
    "Enter Z of fixed point: "
};


static void get_pleqn(struct mged_state *s, fastf_t *plane, const char *argv[]);
static void get_rotfb(struct mged_state *s, fastf_t *plane, const char *argv[], const struct rt_arb_internal *arb);
static void get_nupnt(struct mged_state *s, fastf_t *plane, const char *argv[]);
static int get_3pts(struct mged_state *s, fastf_t *plane, const char *argv[], const struct bn_tol *tol);

/*
 * Redefines one of the defining planes for a GENARB8. Finds which
 * plane to redefine and gets input, then shuttles the process over to
 * one of four functions before calculating new vertices.
 */
int
f_facedef(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    struct cmdtab *ctp = (struct cmdtab *)clientData;
    MGED_CK_CMD(ctp);
    struct mged_state *s = ctp->s;
    short int i;
    int face, prod, plane;
    struct rt_db_internal intern;
    struct rt_arb_internal *arb;
    struct rt_arb_internal *arbo;
    plane_t planes[6];
    int status;
    struct bu_vls error_msg;

    if (argc < 2) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "help facedef");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if (setjmp(jmp_env) == 0)
	(void)signal(SIGINT, sig3);  /* allow interrupts */
    else
	return TCL_OK;

    status = TCL_OK;
    BU_VLS_INIT(&error_msg);
    RT_DB_INTERNAL_INIT(&intern);

    if (s->global_editing_state != ST_S_EDIT) {
	Tcl_AppendResult(interp, "Facedef: must be in solid edit mode\n", (char *)NULL);
	status = TCL_ERROR;
	goto end;
    }
    if (s->s_edit->es_int.idb_type != ID_ARB8) {
	Tcl_AppendResult(interp, "Facedef: solid type must be ARB\n");
	status = TCL_ERROR;
	goto end;
    }

    /* apply s->s_edit->e_mat editing to parameters.  "new way" */
    transform_editing_solid(s, &intern, s->s_edit->e_mat, &s->s_edit->es_int, 0);

    arb = (struct rt_arb_internal *)intern.idb_ptr;
    RT_ARB_CK_MAGIC(arb);

    /* find new planes to account for any editing */
    if (rt_arb_calc_planes(&error_msg, arb, s->es_type, planes, &s->tol.tol)) {
	Tcl_AppendResult(interp, bu_vls_addr(&error_msg),
			 "Unable to determine plane equations\n", (char *)NULL);
	status = TCL_ERROR;
	bu_vls_free(&error_msg);
	goto end;
    }
    bu_vls_free(&error_msg);

    /* get face, initialize args and argcnt */
    face = atoi(argv[1]);

    /* use product of vertices to distinguish faces */
    for (i=0, prod=1;i<4;i++) {
	if (face > 0) {
	    prod *= face%10;
	    face /= 10;
	}
    }

    switch (prod) {
	case 6:			/* face 123 of arb4 */
	case 24:plane=0;	/* face 1234 of arb8 */
	    /* face 1234 of arb7 */
	    /* face 1234 of arb6 */
	    /* face 1234 of arb5 */
	    if (s->es_type==4 && prod==24)
		plane=2; 	/* face 234 of arb4 */
	    break;
	case 8:			/* face 124 of arb4 */
	case 180: 		/* face 2365 of arb6 */
	case 210:		/* face 567 of arb7 */
	case 1680:plane=1;      /* face 5678 of arb8 */
	    break;
	case 30:		/* face 235 of arb5 */
	case 120:		/* face 1564 of arb6 */
	case 20:      		/* face 145 of arb7 */
	case 160:plane=2;	/* face 1584 of arb8 */
	    if (s->es_type==5)
		plane=4; 	/* face 145 of arb5 */
	    break;
	case 12:		/* face 134 of arb4 */
	case 10:		/* face 125 of arb6 */
	case 252:plane=3;	/* face 2376 of arb8 */
	    /* face 2376 of arb7 */
	    if (s->es_type==5)
		plane=1; 	/* face 125 of arb5 */
	    break;
	case 72:               	/* face 346 of arb6 */
	case 60:plane=4;	/* face 1265 of arb8 */
	    /* face 1265 of arb7 */
	    if (s->es_type==5)
		plane=3; 	/* face 345 of arb5 */
	    break;
	case 420:		/* face 4375 of arb7 */
	case 672:plane=5;	/* face 4378 of arb8 */
	    break;
	default:
	    {
		struct bu_vls tmp_vls = BU_VLS_INIT_ZERO;

		bu_vls_printf(&tmp_vls, "bad face (product=%d)\n", prod);
		Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
		bu_vls_free(&tmp_vls);
		status = TCL_ERROR;
		goto end;
	    }
    }

    if (argc < 3) {
	/* menu of choices for plane equation definition */
	Tcl_AppendResult(interp,
			 "\ta   planar equation\n",
			 "\tb   3 points\n",
			 "\tc   rot, fb angles + fixed pt\n",
			 "\td   same plane thru fixed pt\n",
			 "\tq   quit\n\n",
			 MORE_ARGS_STR, "Enter form of new face definition: ", (char *)NULL);
	status = TCL_ERROR;
	goto end;
    }

    switch (argv[2][0]) {
	case 'a':
	    /* special case for arb7, because of 2 4-pt planes meeting */
	    if (s->es_type == 7)
		if (plane!=0 && plane!=3) {
		    Tcl_AppendResult(interp, "Facedef: can't redefine that arb7 plane\n", (char *)NULL);
		    status = TCL_ERROR;
		    goto end;
		}
	    if (argc < 7) {
		/* total # of args under this option */
		Tcl_AppendResult(interp, MORE_ARGS_STR, p_pleqn[argc-3], (char *)NULL);
		status = TCL_ERROR;
		goto end;
	    }
	    get_pleqn(s, planes[plane], &argv[3]);
	    break;
	case 'b':
	    /* special case for arb7, because of 2 4-pt planes meeting */
	    if (s->es_type == 7)
		if (plane!=0 && plane!=3) {
		    Tcl_AppendResult(interp, "Facedef: can't redefine that arb7 plane\n", (char *)NULL);
		    status = TCL_ERROR;
		    goto end;
		}
	    if (argc < 12) {
		/* total # of args under this option */
		struct bu_vls tmp_vls = BU_VLS_INIT_ZERO;

		bu_vls_printf(&tmp_vls, "%s%s %d: ", MORE_ARGS_STR, p_3pts[(argc-3)%3], argc/3);
		Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
		bu_vls_free(&tmp_vls);
		status = TCL_ERROR;
		goto end;
	    }
	    if (get_3pts(s, planes[plane], &argv[3], &s->tol.tol)) {
		status = TCL_ERROR;
		goto end;
	    }
	    break;
	case 'c':
	    /* special case for arb7, because of 2 4-pt planes meeting */
	    if (s->es_type == 7 && (plane != 0 && plane != 3)) {
		if (argc < 5) {
		    /* total # of args under this option */
		    Tcl_AppendResult(interp, MORE_ARGS_STR, p_rotfb[argc-3], (char *)NULL);
		    status = TCL_ERROR;
		    goto end;
		}

		argv[5] = "Release 6";
		Tcl_AppendResult(interp, "Fixed point is vertex five.\n");
	    }
	    /* total # of as under this option */
	    else if (argc < 8 && (argc > 5 ? argv[5][0] != 'R' : 1)) {
		Tcl_AppendResult(interp, MORE_ARGS_STR, p_rotfb[argc-3], (char *)NULL);
		status = TCL_ERROR;
		goto end;
	    }
	    get_rotfb(s, planes[plane], &argv[3], arb);
	    break;
	case 'd':
	    /* special case for arb7, because of 2 4-pt planes meeting */
	    if (s->es_type == 7)
		if (plane!=0 && plane!=3) {
		    Tcl_AppendResult(interp, "Facedef: can't redefine that arb7 plane\n", (char *)NULL);
		    status = TCL_ERROR;
		    goto end;
		}
	    if (argc < 6) {
		/* total # of args under this option */
		Tcl_AppendResult(interp, MORE_ARGS_STR, p_nupnt[argc-3], (char *)NULL);
		status = TCL_ERROR;
		goto end;
	    }
	    get_nupnt(s, planes[plane], &argv[3]);
	    break;
	case 'q':
	    return TCL_OK;
	default:
	    Tcl_AppendResult(interp, "Facedef: '", argv[2], "' is not an option\n", (char *)NULL);
	    status = TCL_ERROR;
	    goto end;
    }

    /* find all vertices from the plane equations */
    if (rt_arb_calc_points(arb, s->es_type, (const plane_t *)planes, &s->tol.tol) < 0) {
	Tcl_AppendResult(interp, "facedef:  unable to find points\n", (char *)NULL);
	status = TCL_ERROR;
	goto end;
    }
    /* Now have 8 points, which is the internal form of an ARB8. */

    /* Transform points back before s->s_edit->e_mat changes */
    /* This is the "new way" */
    arbo = (struct rt_arb_internal *)s->s_edit->es_int.idb_ptr;
    RT_ARB_CK_MAGIC(arbo);

    for (i=0; i<8; i++) {
	MAT4X3PNT(arbo->pt[i], s->s_edit->e_invmat, arb->pt[i]);
    }
    rt_db_free_internal(&intern);

    /* draw the new solid */
    replot_editing_solid(s);

 end:
    (void)signal(SIGINT, SIG_IGN);
    return status;
}


/*
 * Gets the planar equation from the array argv[] and puts the result
 * into 'plane'.
 */
static void
get_pleqn(struct mged_state *s, fastf_t *plane, const char *argv[])
{
    int i;

    if (s->dbip == DBI_NULL)
	return;

    for (i=0; i<4; i++)
	plane[i]= atof(argv[i]);
    VUNITIZE(&plane[0]);
    plane[W] *= s->dbip->dbi_local2base;
    return;
}


/*
 * Gets three definite points from the array argv[] and finds the
 * planar equation from these points.  The resulting plane equation is
 * stored in 'plane'.
 *
 * Returns -
 * 0 success
 * -1 failure
 */
static int
get_3pts(struct mged_state *s, fastf_t *plane, const char *argv[], const struct bn_tol *tol)
{
    int i;
    point_t a, b, c;

    CHECK_DBI_NULL;

    for (i=0; i<3; i++)
	a[i] = atof(argv[0+i]) * s->dbip->dbi_local2base;
    for (i=0; i<3; i++)
	b[i] = atof(argv[3+i]) * s->dbip->dbi_local2base;
    for (i=0; i<3; i++)
	c[i] = atof(argv[6+i]) * s->dbip->dbi_local2base;

    if (bg_make_plane_3pnts(plane, a, b, c, tol) < 0) {
	Tcl_AppendResult(s->interp, "Facedef: not a plane\n", (char *)NULL);
	return -1;		/* failure */
    }
    return 0;			/* success */
}


/*
 * Gets information from the array argv[].  Finds the planar equation
 * given rotation and fallback angles, plus a fixed point. Result is
 * stored in 'plane'. The vertices pointed to by 's_recp' are used if
 * a vertex is chosen as fixed point.
 */
static void
get_rotfb(struct mged_state *s, fastf_t *plane, const char *argv[], const struct rt_arb_internal *arb)
{
    fastf_t rota, fb_a;
    short int i, temp;
    point_t pt;

    if (s->dbip == DBI_NULL)
	return;

    rota= atof(argv[0]) * DEG2RAD;
    fb_a  = atof(argv[1]) * DEG2RAD;

    /* calculate normal vector (length=1) from rot, struct fb */
    plane[0] = cos(fb_a) * cos(rota);
    plane[1] = cos(fb_a) * sin(rota);
    plane[2] = sin(fb_a);

    if (argv[2][0] == 'v') {
	/* vertex given */
	/* strip off 'v', subtract 1 */
	temp = atoi(argv[2]+1) - 1;
	plane[W]= VDOT(&plane[0], arb->pt[temp]);
    } else {
	/* definite point given */
	for (i=0; i<3; i++)
	    pt[i]=atof(argv[2+i]) * s->dbip->dbi_local2base;
	plane[W]=VDOT(&plane[0], pt);
    }
}


/*
 * Gets a point from the three strings in the 'argv' array.  The value
 * of D of 'plane' is changed such that the plane passes through the
 * input point.
 */
static void
get_nupnt(struct mged_state *s, fastf_t *plane, const char *argv[])
{
    int i;
    point_t pt;

    if (s->dbip == DBI_NULL)
	return;

    for (i=0; i<3; i++)
	pt[i] = atof(argv[i]) * s->dbip->dbi_local2base;
    plane[W] = VDOT(&plane[0], pt);
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
