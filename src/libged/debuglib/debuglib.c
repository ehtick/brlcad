/*                         D E B U G L I B . C
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
/** @file libged/debuglib.c
 *
 * The debuglib command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "bu/debug.h"
#include "../ged_private.h"


int
ged_debuglib_core(struct ged *gedp, int argc, const char *argv[])
{
    static const char *usage = "[hex_code]";

    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    if (argc > 2) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    /* get librt's debug bit vector */
    if (argc == 1) {
	bu_vls_printb(gedp->ged_result_str, "Possible flags", 0xffffffffL, RT_DEBUG_FORMAT);
	bu_vls_printf(gedp->ged_result_str, "\n");
    } else {
	/* set librt's debug bit vector */
	if (sscanf(argv[1], "%x", (unsigned int *)&rt_debug) != 1) {
	    bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	    return BRLCAD_ERROR;
	}
    }

    bu_vls_printb(gedp->ged_result_str, "librt RT_G_DEBUG", RT_G_DEBUG, RT_DEBUG_FORMAT);
    bu_vls_printf(gedp->ged_result_str, "\n");

    return BRLCAD_OK;
}


#ifdef GED_PLUGIN
#include "../include/plugin.h"
struct ged_cmd_impl debuglib_cmd_impl = {
    "debuglib",
    ged_debuglib_core,
    GED_CMD_DEFAULT
};

const struct ged_cmd debuglib_cmd = { &debuglib_cmd_impl };
const struct ged_cmd *debuglib_cmds[] = { &debuglib_cmd, NULL };

static const struct ged_plugin pinfo = { GED_API,  debuglib_cmds, 1 };

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
