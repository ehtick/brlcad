/*                         R F A R B . C
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
/** @file libged/rfarb.c
 *
 * The rfarb command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "rt/geom.h"
#include "../ged_private.h"


int
ged_rfarb_core(struct ged *gedp, int argc, const char *argv[])
{
    struct directory *dp;
    struct rt_db_internal internal;
    struct rt_arb_internal *aip;

    int i;
    int solve[3];
    vect_t norm;
    fastf_t ndotv;

    /* intentionally double for scan */
    double known_pt[3];
    double pt[3][2];
    double thick;
    double rota;
    double fba;

    static const char *usage = "name pX pY pZ rA fbA c X Y c X Y c X Y th";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (argc != 16) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (db_lookup(gedp->dbip, argv[1], LOOKUP_QUIET) != RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "%s: %s already exists\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &known_pt[X]) != 1 ||
	sscanf(argv[3], "%lf", &known_pt[Y]) != 1 ||
	sscanf(argv[4], "%lf", &known_pt[Z]) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad value - %s %s %s",
		      argv[0], argv[2], argv[3], argv[4]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[5], "%lf", &rota) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad rotation angle - %s", argv[0], argv[5]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[6], "%lf", &fba) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad fallback angle - %s", argv[0], argv[6]);
	return BRLCAD_ERROR;
    }

    rota *= DEG2RAD;
    fba *= DEG2RAD;

    /* calculate plane defined by these angles */
    norm[0] = cos(fba) * cos(rota);
    norm[1] = cos(fba) * sin(rota);
    norm[2] = sin(fba);

    for (i = 0; i < 3; i++) {
	switch (argv[7+3*i][0]) {
	    case 'x':
		if (ZERO(norm[0])) {
		    bu_vls_printf(gedp->ged_result_str, "X not unique in this face\n");
		    return BRLCAD_ERROR;
		}
		solve[i] = X;

		if (sscanf(argv[7+3*i+1], "%lf", &pt[i][X]) != 1 ||
		    sscanf(argv[7+3*i+2], "%lf", &pt[i][Y]) != 1) {
		    bu_vls_printf(gedp->ged_result_str, "%s: at least one bad value - %s %s",
				  argv[0], argv[7+3*i+1], argv[7+3*i+2]);
		}

		pt[i][X] *= gedp->dbip->dbi_local2base;
		pt[i][Y] *= gedp->dbip->dbi_local2base;
		break;

	    case 'y':
		if (ZERO(norm[1])) {
		    bu_vls_printf(gedp->ged_result_str, "Y not unique in this face\n");
		    return BRLCAD_ERROR;
		}
		solve[i] = Y;

		if (sscanf(argv[7+3*i+1], "%lf", &pt[i][X]) != 1 ||
		    sscanf(argv[7+3*i+2], "%lf", &pt[i][Y]) != 1) {
		    bu_vls_printf(gedp->ged_result_str, "%s: at least one bad value - %s %s",
				  argv[0], argv[7+3*i+1], argv[7+3*i+2]);
		}

		pt[i][X] *= gedp->dbip->dbi_local2base;
		pt[i][Y] *= gedp->dbip->dbi_local2base;
		break;

	    case 'z':
		if (ZERO(norm[2])) {
		    bu_vls_printf(gedp->ged_result_str, "Z not unique in this face\n");
		    return BRLCAD_ERROR;
		}
		solve[i] = Z;

		if (sscanf(argv[7+3*i+1], "%lf", &pt[i][X]) != 1 ||
		    sscanf(argv[7+3*i+2], "%lf", &pt[i][Y]) != 1) {
		    bu_vls_printf(gedp->ged_result_str, "%s: at least one bad value - %s %s",
				  argv[0], argv[7+3*i+1], argv[7+3*i+2]);
		}

		pt[i][X] *= gedp->dbip->dbi_local2base;
		pt[i][Y] *= gedp->dbip->dbi_local2base;
		break;

	    default:
		bu_vls_printf(gedp->ged_result_str, "coordinate must be x, y, or z\n");
		return BRLCAD_ERROR;
	}
    }

    if (sscanf(argv[7+3*3], "%lf", &thick) != 1 || ZERO(thick)) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad thickness - %s", argv[0], argv[7+3*3]);
	return BRLCAD_ERROR;
    }
    thick *= gedp->dbip->dbi_local2base;

    RT_DB_INTERNAL_INIT(&internal);
    internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
    internal.idb_type = ID_ARB8;
    internal.idb_meth = &OBJ[ID_ARB8];
    BU_ALLOC(internal.idb_ptr, struct rt_arb_internal);
    aip = (struct rt_arb_internal *)internal.idb_ptr;
    aip->magic = RT_ARB_INTERNAL_MAGIC;

    for (i = 0; i < 8; i++) {
	VSET(aip->pt[i], 0.0, 0.0, 0.0);
    }

    VSCALE(aip->pt[0], known_pt, gedp->dbip->dbi_local2base);

    ndotv = VDOT(aip->pt[0], norm);

    /* calculate the unknown coordinate for points 2, 3, 4 */
    for (i = 0; i < 3; i++) {
	int j;
	j = i+1;

	switch (solve[i]) {
	    case X:
		aip->pt[j][Y] = pt[i][0];
		aip->pt[j][Z] = pt[i][1];
		aip->pt[j][X] = (ndotv
				 - norm[1] * aip->pt[j][Y]
				 - norm[2] * aip->pt[j][Z])
		    / norm[0];
		break;
	    case Y:
		aip->pt[j][X] = pt[i][0];
		aip->pt[j][Z] = pt[i][1];
		aip->pt[j][Y] = (ndotv
				 - norm[0] * aip->pt[j][X]
				 - norm[2] * aip->pt[j][Z])
		    / norm[1];
		break;
	    case Z:
		aip->pt[j][X] = pt[i][0];
		aip->pt[j][Y] = pt[i][1];
		aip->pt[j][Z] = (ndotv
				 - norm[0] * aip->pt[j][X]
				 - norm[1] * aip->pt[j][Y])
		    / norm[2];
		break;

	    default:
		return BRLCAD_ERROR;
	}
    }

    /* calculate the remaining 4 vertices */
    for (i = 0; i < 4; i++) {
	VJOIN1(aip->pt[i+4], aip->pt[i], thick, norm);
    }

    dp = db_diradd(gedp->dbip, argv[1], RT_DIR_PHONY_ADDR, 0, RT_DIR_SOLID, (void *)&internal.idb_type);
    if (dp == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "%s: Cannot add %s to the directory\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    if (rt_db_put_internal(dp, gedp->dbip, &internal, &rt_uniresource) < 0) {
	rt_db_free_internal(&internal);
	bu_vls_printf(gedp->ged_result_str, "%s: Database write error, aborting.\n", argv[0]);
    }

    return BRLCAD_OK;
}


#ifdef GED_PLUGIN
#include "../include/plugin.h"
struct ged_cmd_impl rfarb_cmd_impl = {
    "rfarb",
    ged_rfarb_core,
    GED_CMD_DEFAULT
};

const struct ged_cmd rfarb_cmd = { &rfarb_cmd_impl };
const struct ged_cmd *rfarb_cmds[] = { &rfarb_cmd, NULL };

static const struct ged_plugin pinfo = { GED_API,  rfarb_cmds, 1 };

COMPILER_DLLEXPORT const struct ged_plugin *ged_plugin_info(void)
{
    return &pinfo;
}
#endif /* GED_PLUGIN */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
