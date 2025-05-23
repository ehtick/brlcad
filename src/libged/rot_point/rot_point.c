/*                         R O T _ P O I N T . C
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
/** @file libged/rot_point.c
 *
 * The rot_point command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../ged_private.h"


int
ged_rot_point_core(struct ged *gedp, int argc, const char *argv[])
{
    mat_t invRot;
    point_t point;
    point_t rpoint;
    static const char *usage = "x y z";

    GED_CHECK_VIEW(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (argc != 2 && argc != 4) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (argc == 2) {
	if (bn_decode_vect(point, argv[1]) != 3) {
	    bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	    return BRLCAD_ERROR;
	}
    } else {
	double scan[3];

	if (sscanf(argv[1], "%lf", &scan[X]) != 1) {
	    bu_vls_printf(gedp->ged_result_str, "ged_m2v_point: bad X value - %s\n", argv[1]);
	    return BRLCAD_ERROR;
	}

	if (sscanf(argv[2], "%lf", &scan[Y]) != 1) {
	    bu_vls_printf(gedp->ged_result_str, "ged_m2v_point: bad Y value - %s\n", argv[2]);
	    return BRLCAD_ERROR;
	}

	if (sscanf(argv[3], "%lf", &scan[Z]) != 1) {
	    bu_vls_printf(gedp->ged_result_str, "ged_m2v_point: bad Z value - %s\n", argv[3]);
	    return BRLCAD_ERROR;
	}

	/* convert from double to fastf_t */
	VMOVE(point, scan);
    }

    /* Rotate the incoming point */
    bn_mat_inv(invRot, gedp->ged_gvp->gv_rotation);
    MAT4X3PNT(rpoint, invRot, point);
    bn_encode_vect(gedp->ged_result_str, rpoint, 1);

    return BRLCAD_OK;
}


#ifdef GED_PLUGIN
#include "../include/plugin.h"
struct ged_cmd_impl rot_point_cmd_impl = {
    "rot_point",
    ged_rot_point_core,
    GED_CMD_DEFAULT
};

const struct ged_cmd rot_point_cmd = { &rot_point_cmd_impl };
const struct ged_cmd *rot_point_cmds[] = { &rot_point_cmd, NULL };

static const struct ged_plugin pinfo = { GED_API,  rot_point_cmds, 1 };

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
