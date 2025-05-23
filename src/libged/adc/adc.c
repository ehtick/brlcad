/*                           A D C . C
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
/** @file libged/adc.c
 *
 */

#include "common.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>


#include "vmath.h"
#include "ged.h"

void ged_calc_adc_pos(struct bview *gvp);
void ged_calc_adc_a1(struct bview *gvp);
void ged_calc_adc_a2(struct bview *gvp);
void ged_calc_adc_dst(struct bview *gvp);

static void
adc_vls_print(struct bview *gvp, fastf_t base2local, struct bu_vls *out_vp)
{
    bu_vls_printf(out_vp, "draw = %d\n", gvp->gv_s->gv_adc.draw);
    bu_vls_printf(out_vp, "a1 = %.15e\n", gvp->gv_s->gv_adc.a1);
    bu_vls_printf(out_vp, "a2 = %.15e\n", gvp->gv_s->gv_adc.a2);
    bu_vls_printf(out_vp, "dst = %.15e\n", gvp->gv_s->gv_adc.dst * gvp->gv_scale * base2local);
    bu_vls_printf(out_vp, "odst = %d\n", gvp->gv_s->gv_adc.dv_dist);
    bu_vls_printf(out_vp, "hv = %.15e %.15e\n",
		  gvp->gv_s->gv_adc.pos_grid[X] * gvp->gv_scale * base2local,
		  gvp->gv_s->gv_adc.pos_grid[Y] * gvp->gv_scale * base2local);
    bu_vls_printf(out_vp, "xyz = %.15e %.15e %.15e\n",
		  gvp->gv_s->gv_adc.pos_model[X] * base2local,
		  gvp->gv_s->gv_adc.pos_model[Y] * base2local,
		  gvp->gv_s->gv_adc.pos_model[Z] * base2local);
    bu_vls_printf(out_vp, "x = %d\n", gvp->gv_s->gv_adc.dv_x);
    bu_vls_printf(out_vp, "y = %d\n", gvp->gv_s->gv_adc.dv_y);
    bu_vls_printf(out_vp, "anchor_pos = %d\n", gvp->gv_s->gv_adc.anchor_pos);
    bu_vls_printf(out_vp, "anchor_a1 = %d\n", gvp->gv_s->gv_adc.anchor_a1);
    bu_vls_printf(out_vp, "anchor_a2 = %d\n", gvp->gv_s->gv_adc.anchor_a2);
    bu_vls_printf(out_vp, "anchor_dst = %d\n", gvp->gv_s->gv_adc.anchor_dst);
    bu_vls_printf(out_vp, "anchorpoint_a1 = %.15e %.15e %.15e\n",
		  gvp->gv_s->gv_adc.anchor_pt_a1[X] * base2local,
		  gvp->gv_s->gv_adc.anchor_pt_a1[Y] * base2local,
		  gvp->gv_s->gv_adc.anchor_pt_a1[Z] * base2local);
    bu_vls_printf(out_vp, "anchorpoint_a2 = %.15e %.15e %.15e\n",
		  gvp->gv_s->gv_adc.anchor_pt_a2[X] * base2local,
		  gvp->gv_s->gv_adc.anchor_pt_a2[Y] * base2local,
		  gvp->gv_s->gv_adc.anchor_pt_a2[Z] * base2local);
    bu_vls_printf(out_vp, "anchorpoint_dst = %.15e %.15e %.15e\n",
		  gvp->gv_s->gv_adc.anchor_pt_dst[X] * base2local,
		  gvp->gv_s->gv_adc.anchor_pt_dst[Y] * base2local,
		  gvp->gv_s->gv_adc.anchor_pt_dst[Z] * base2local);
}


static void
adc_usage(struct bu_vls *vp, const char *name)
{
    bu_vls_printf(vp, "Usage: %s \n", name);
    bu_vls_printf(vp, "%s", " adc vars                      print a list of all variables (i.e. var = val)\n");
    bu_vls_printf(vp, "%s", " adc draw [0|1]                set or get the draw parameter\n");
    bu_vls_printf(vp, "%s", " adc a1   [#]                  set or get angle1\n");
    bu_vls_printf(vp, "%s", " adc a2   [#]                  set or get angle2\n");
    bu_vls_printf(vp, "%s", " adc dst  [#]                  set or get radius (distance) of tick\n");
    bu_vls_printf(vp, "%s", " adc odst [#]                  set or get radius (distance) of tick (+-2047)\n");
    bu_vls_printf(vp, "%s", " adc hv   [# #]                set or get position (grid coordinates)\n");
    bu_vls_printf(vp, "%s", " adc xyz  [# # #]              set or get position (model coordinates)\n");
    bu_vls_printf(vp, "%s", " adc x [#]                     set or get horizontal position (+-2047)\n");
    bu_vls_printf(vp, "%s", " adc y [#]                     set or get vertical position (+-2047)\n");
    bu_vls_printf(vp, "%s", " adc dh #                      add to horizontal position (grid coordinates)\n");
    bu_vls_printf(vp, "%s", " adc dv #                      add to vertical position (grid coordinates)\n");
    bu_vls_printf(vp, "%s", " adc dx #                      add to X position (model coordinates)\n");
    bu_vls_printf(vp, "%s", " adc dy #                      add to Y position (model coordinates)\n");
    bu_vls_printf(vp, "%s", " adc dz #                      add to Z position (model coordinates)\n");
    bu_vls_printf(vp, "%s", " adc anchor_pos [0|1]          anchor ADC to current position in model coordinates\n");
    bu_vls_printf(vp, "%s", " adc anchor_a1  [0|1]          anchor angle1 to go through anchorpoint_a1\n");
    bu_vls_printf(vp, "%s", " adc anchor_a2  [0|1]          anchor angle2 to go through anchorpoint_a2\n");
    bu_vls_printf(vp, "%s", " adc anchor_dst [0|1]          anchor tick distance to go through anchorpoint_dst\n");
    bu_vls_printf(vp, "%s", " adc anchorpoint_a1  [# # #]   set or get anchor point for angle1\n");
    bu_vls_printf(vp, "%s", " adc anchorpoint_a2  [# # #]   set or get anchor point for angle2\n");
    bu_vls_printf(vp, "%s", " adc anchorpoint_dst [# # #]   set or get anchor point for tick distance\n");
    bu_vls_printf(vp, "%s", " adc -i                        any of the above appropriate commands will interpret parameters as increments\n");
    bu_vls_printf(vp, "%s", " adc reset                     reset angles, location, and tick distance\n");
    bu_vls_printf(vp, "%s", " adc help                      prints this help message\n");
}


/*
 * Note - this needs to be rewritten to accept keyword/value pairs so
 * that multiple attributes can be set with a single command call.
 */
int
ged_adc_core(struct ged *gedp,
	int argc,
	const char *argv[])
{
    char *command;
    char *parameter;
    char **argp = (char **)argv;
    double scanval;
    point_t user_pt;		/* Value(s) provided by user */
    point_t scaled_pos;
    int incr_flag;
    int i;

    GED_CHECK_VIEW(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    struct bview *v = gedp->ged_gvp;
    fastf_t gv_scale = (gedp->dbip) ? v->gv_scale * gedp->dbip->dbi_base2local : v->gv_scale;
    double sval = (gedp->dbip) ? gedp->dbip->dbi_local2base : 1.0;

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    if (argc < 2 || 6 < argc) {
	adc_usage(gedp->ged_result_str, argv[0]);
	return BRLCAD_ERROR;
    }

    command = (char *)argv[0];

    if (BU_STR_EQUAL(argv[1], "-i")) {
	if (argc < 5) {
	    bu_vls_printf(gedp->ged_result_str, "%s: -i option specified without an op-val pair", command);
	    return BRLCAD_ERROR;
	}

	incr_flag = 1;
	parameter = (char *)argv[2];
	argc -= 3;
	argp += 3;
    } else {
	incr_flag = 0;
	parameter = (char *)argv[1];
	argc -= 2;
	argp += 2;
    }

    for (i = 0; i < argc; ++i) {
	if (sscanf(argp[i], "%lf", &scanval) != 1) {
	    adc_usage(gedp->ged_result_str, command);
	    return BRLCAD_ERROR;
	}
	user_pt[i] = scanval;
    }

    if (BU_STR_EQUAL(parameter, "draw")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.draw);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i)
		gedp->ged_gvp->gv_s->gv_adc.draw = 1;
	    else
		gedp->ged_gvp->gv_s->gv_adc.draw = 0;

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s draw' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "a1")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%g", gedp->ged_gvp->gv_s->gv_adc.a1);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_a1) {
		if (incr_flag)
		    gedp->ged_gvp->gv_s->gv_adc.a1 += user_pt[0];
		else
		    gedp->ged_gvp->gv_s->gv_adc.a1 = user_pt[0];

		gedp->ged_gvp->gv_s->gv_adc.dv_a1 = (1.0 - (gedp->ged_gvp->gv_s->gv_adc.a1 / 45.0)) * BV_MAX;
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s a1' command accepts only 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "a2")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%g", gedp->ged_gvp->gv_s->gv_adc.a2);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_a2) {
		if (incr_flag)
		    gedp->ged_gvp->gv_s->gv_adc.a2 += user_pt[0];
		else
		    gedp->ged_gvp->gv_s->gv_adc.a2 = user_pt[0];

		gedp->ged_gvp->gv_s->gv_adc.dv_a2 = (1.0 - (gedp->ged_gvp->gv_s->gv_adc.a2 / 45.0)) * BV_MAX;
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s a2' command accepts only 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "dst")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%g", v->gv_s->gv_adc.dst * gv_scale);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_dst) {
		if (incr_flag)
		    gedp->ged_gvp->gv_s->gv_adc.dst += user_pt[0] / gv_scale;
		else
		    gedp->ged_gvp->gv_s->gv_adc.dst = user_pt[0] / gv_scale;

		gedp->ged_gvp->gv_s->gv_adc.dv_dist = (gedp->ged_gvp->gv_s->gv_adc.dst / M_SQRT1_2 - 1.0) * BV_MAX;
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s dst' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "odst")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.dv_dist);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_dst) {
		if (incr_flag)
		    gedp->ged_gvp->gv_s->gv_adc.dv_dist += user_pt[0];
		else
		    gedp->ged_gvp->gv_s->gv_adc.dv_dist = user_pt[0];

		gedp->ged_gvp->gv_s->gv_adc.dst = (gedp->ged_gvp->gv_s->gv_adc.dv_dist * INV_BV + 1.0) * M_SQRT1_2;
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s odst' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "dh")) {
	if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		gedp->ged_gvp->gv_s->gv_adc.pos_grid[X] += user_pt[0] / gv_scale;
		adc_grid_to_adc_view(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_view2model, BV_MAX);
		MAT4X3PNT(gedp->ged_gvp->gv_s->gv_adc.pos_model, gedp->ged_gvp->gv_view2model, gedp->ged_gvp->gv_s->gv_adc.pos_view);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s dh' command requires 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "dv")) {
	if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		gedp->ged_gvp->gv_s->gv_adc.pos_grid[Y] += user_pt[0] / gv_scale;
		adc_grid_to_adc_view(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_view2model, BV_MAX);
		MAT4X3PNT(gedp->ged_gvp->gv_s->gv_adc.pos_model, gedp->ged_gvp->gv_view2model, gedp->ged_gvp->gv_s->gv_adc.pos_view);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s dv' command requires 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "hv")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%g %g",
			  gedp->ged_gvp->gv_s->gv_adc.pos_grid[X] * gv_scale,
			  gedp->ged_gvp->gv_s->gv_adc.pos_grid[Y] * gv_scale);
	    return BRLCAD_OK;
	} else if (argc == 2) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		if (incr_flag) {
		    gedp->ged_gvp->gv_s->gv_adc.pos_grid[X] += user_pt[X] / gv_scale;
		    gedp->ged_gvp->gv_s->gv_adc.pos_grid[Y] += user_pt[Y] / gv_scale;
		} else {
		    gedp->ged_gvp->gv_s->gv_adc.pos_grid[X] = user_pt[X] / gv_scale;
		    gedp->ged_gvp->gv_s->gv_adc.pos_grid[Y] = user_pt[Y] / gv_scale;
		}

		gedp->ged_gvp->gv_s->gv_adc.pos_grid[Z] = 0.0;
		adc_grid_to_adc_view(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_view2model, BV_MAX);
		MAT4X3PNT(gedp->ged_gvp->gv_s->gv_adc.pos_model, gedp->ged_gvp->gv_view2model, gedp->ged_gvp->gv_s->gv_adc.pos_model);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s hv' command requires 0 or 2 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "dx")) {
	if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		double pval = (gedp->dbip) ? (user_pt[0] * gedp->dbip->dbi_local2base) : user_pt[0];
		gedp->ged_gvp->gv_s->gv_adc.pos_model[X] += pval;
		adc_model_to_adc_view(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view, BV_MAX);
		adc_view_to_adc_grid(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s dx' command requires 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "dy")) {
	if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		double pval = (gedp->dbip) ? (user_pt[0] * gedp->dbip->dbi_local2base) : user_pt[0];
		gedp->ged_gvp->gv_s->gv_adc.pos_model[Y] += pval;
		adc_model_to_adc_view(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view, BV_MAX);
		adc_view_to_adc_grid(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s dy' command requires 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "dz")) {
	if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		double pval = (gedp->dbip) ? (user_pt[0] * gedp->dbip->dbi_local2base) : user_pt[0];
		gedp->ged_gvp->gv_s->gv_adc.pos_model[Z] += pval;
		adc_model_to_adc_view(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view, BV_MAX);
		adc_view_to_adc_grid(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s dz' command requires 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "xyz")) {
	if (argc == 0) {
	    VSCALE(scaled_pos, gedp->ged_gvp->gv_s->gv_adc.pos_model, sval);
	    bu_vls_printf(gedp->ged_result_str, "%g %g %g", V3ARGS(scaled_pos));
	    return BRLCAD_OK;
	} else if (argc == 3) {
	    VSCALE(user_pt, user_pt, sval);

	    if (incr_flag) {
		VADD2(gedp->ged_gvp->gv_s->gv_adc.pos_model, gedp->ged_gvp->gv_s->gv_adc.pos_model, user_pt);
	    } else {
		VMOVE(gedp->ged_gvp->gv_s->gv_adc.pos_model, user_pt);
	    }

	    adc_model_to_adc_view(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view, BV_MAX);
	    adc_view_to_adc_grid(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s xyz' command requires 0 or 3 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "x")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.dv_x);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		if (incr_flag) {
		    gedp->ged_gvp->gv_s->gv_adc.dv_x += user_pt[0];
		} else {
		    gedp->ged_gvp->gv_s->gv_adc.dv_x = user_pt[0];
		}

		gedp->ged_gvp->gv_s->gv_adc.pos_view[X] = gedp->ged_gvp->gv_s->gv_adc.dv_x * INV_BV;
		gedp->ged_gvp->gv_s->gv_adc.pos_view[Y] = gedp->ged_gvp->gv_s->gv_adc.dv_y * INV_BV;
		adc_view_to_adc_grid(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view);
		MAT4X3PNT(gedp->ged_gvp->gv_s->gv_adc.pos_model, gedp->ged_gvp->gv_view2model, gedp->ged_gvp->gv_s->gv_adc.pos_view);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s x' command requires 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "y")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.dv_y);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    if (!gedp->ged_gvp->gv_s->gv_adc.anchor_pos) {
		if (incr_flag) {
		    gedp->ged_gvp->gv_s->gv_adc.dv_y += user_pt[0];
		} else {
		    gedp->ged_gvp->gv_s->gv_adc.dv_y = user_pt[0];
		}

		gedp->ged_gvp->gv_s->gv_adc.pos_view[X] = gedp->ged_gvp->gv_s->gv_adc.dv_x * INV_BV;
		gedp->ged_gvp->gv_s->gv_adc.pos_view[Y] = gedp->ged_gvp->gv_s->gv_adc.dv_y * INV_BV;
		adc_view_to_adc_grid(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_model2view);
		MAT4X3PNT(gedp->ged_gvp->gv_s->gv_adc.pos_model, gedp->ged_gvp->gv_view2model, gedp->ged_gvp->gv_s->gv_adc.pos_view);
	    }

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s y' command requires 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "anchor_pos")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.anchor_pos);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i < 0 || 2 < i) {
		bu_vls_printf(gedp->ged_result_str, "The '%d anchor_pos' parameter accepts values of 0, 1, or 2.", i);
		return BRLCAD_ERROR;
	    }

	    gedp->ged_gvp->gv_s->gv_adc.anchor_pos = i;
	    ged_calc_adc_pos(gedp->ged_gvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s anchor_pos' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "anchor_a1")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.anchor_a1);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i)
		gedp->ged_gvp->gv_s->gv_adc.anchor_a1 = 1;
	    else
		gedp->ged_gvp->gv_s->gv_adc.anchor_a1 = 0;

	    ged_calc_adc_a1(gedp->ged_gvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s anchor_a1' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "anchorpoint_a1")) {
	if (argc == 0) {
	    VSCALE(scaled_pos, gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a1, sval);
	    bu_vls_printf(gedp->ged_result_str, "%g %g %g", V3ARGS(scaled_pos));

	    return BRLCAD_OK;
	} else if (argc == 3) {
	    VSCALE(user_pt, user_pt, sval);

	    if (incr_flag) {
		VADD2(gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a1, gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a1, user_pt);
	    } else {
		VMOVE(gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a1, user_pt);
	    }

	    ged_calc_adc_a1(gedp->ged_gvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s anchorpoint_a1' command accepts 0 or 3 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "anchor_a2")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.anchor_a2);

	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i)
		gedp->ged_gvp->gv_s->gv_adc.anchor_a2 = 1;
	    else
		gedp->ged_gvp->gv_s->gv_adc.anchor_a2 = 0;

	    ged_calc_adc_a2(gedp->ged_gvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s anchor_a2' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "anchorpoint_a2")) {
	if (argc == 0) {
	    VSCALE(scaled_pos, gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a2, sval);

	    bu_vls_printf(gedp->ged_result_str, "%g %g %g", V3ARGS(scaled_pos));

	    return BRLCAD_OK;
	} else if (argc == 3) {
	    VSCALE(user_pt, user_pt, sval);

	    if (incr_flag) {
		VADD2(gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a2, gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a2, user_pt);
	    } else {
		VMOVE(gedp->ged_gvp->gv_s->gv_adc.anchor_pt_a2, user_pt);
	    }

	    ged_calc_adc_a2(gedp->ged_gvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s anchorpoint_a2' command accepts 0 or 3 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "anchor_dst")) {
	if (argc == 0) {
	    bu_vls_printf(gedp->ged_result_str, "%d", gedp->ged_gvp->gv_s->gv_adc.anchor_dst);

	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i) {
		gedp->ged_gvp->gv_s->gv_adc.anchor_dst = 1;
	    } else
		gedp->ged_gvp->gv_s->gv_adc.anchor_dst = 0;

	    ged_calc_adc_dst(gedp->ged_gvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s anchor_dst' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "anchorpoint_dst")) {
	if (argc == 0) {
	    VSCALE(scaled_pos, gedp->ged_gvp->gv_s->gv_adc.anchor_pt_dst, sval);
	    bu_vls_printf(gedp->ged_result_str, "%g %g %g", V3ARGS(scaled_pos));

	    return BRLCAD_OK;
	} else if (argc == 3) {
	    VSCALE(user_pt, user_pt, sval);

	    if (incr_flag) {
		VADD2(gedp->ged_gvp->gv_s->gv_adc.anchor_pt_dst, gedp->ged_gvp->gv_s->gv_adc.anchor_pt_dst, user_pt);
	    } else {
		VMOVE(gedp->ged_gvp->gv_s->gv_adc.anchor_pt_dst, user_pt);
	    }

	    ged_calc_adc_dst(gedp->ged_gvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s anchorpoint_dst' command accepts 0 or 3 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "reset")) {
	if (argc == 0) {
	    adc_reset(&(gedp->ged_gvp->gv_s->gv_adc), gedp->ged_gvp->gv_view2model, gedp->ged_gvp->gv_model2view);

	    return BRLCAD_OK;
	}

	bu_vls_printf(gedp->ged_result_str, "The '%s reset' command accepts no arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (BU_STR_EQUAL(parameter, "vars")) {
	adc_vls_print(gedp->ged_gvp, sval, gedp->ged_result_str);
	return BRLCAD_OK;
    }

    if (BU_STR_EQUAL(parameter, "help")) {
	adc_usage(gedp->ged_result_str, command);
	return GED_HELP;
    }

    bu_vls_printf(gedp->ged_result_str, "%s: unrecognized command '%s'\n", command, parameter);
    adc_usage(gedp->ged_result_str, command);

    return BRLCAD_ERROR;
}


void
ged_calc_adc_pos(struct bview *gvp)
{
    if (gvp->gv_s->gv_adc.anchor_pos == 1) {
	adc_model_to_adc_view(&(gvp->gv_s->gv_adc), gvp->gv_model2view, BV_MAX);
	adc_view_to_adc_grid(&(gvp->gv_s->gv_adc), gvp->gv_model2view);
    } else if (gvp->gv_s->gv_adc.anchor_pos == 2) {
	adc_grid_to_adc_view(&(gvp->gv_s->gv_adc), gvp->gv_view2model, BV_MAX);
	MAT4X3PNT(gvp->gv_s->gv_adc.pos_model, gvp->gv_view2model, gvp->gv_s->gv_adc.pos_view);
    } else {
	adc_view_to_adc_grid(&(gvp->gv_s->gv_adc), gvp->gv_model2view);
	MAT4X3PNT(gvp->gv_s->gv_adc.pos_model, gvp->gv_view2model, gvp->gv_s->gv_adc.pos_view);
    }
}


void
ged_calc_adc_a1(struct bview *gvp)
{
    if (gvp->gv_s->gv_adc.anchor_a1) {
	fastf_t dx, dy;
	point_t view_pt;

	MAT4X3PNT(view_pt, gvp->gv_model2view, gvp->gv_s->gv_adc.anchor_pt_a1);
	dx = view_pt[X] * BV_MAX - gvp->gv_s->gv_adc.dv_x;
	dy = view_pt[Y] * BV_MAX - gvp->gv_s->gv_adc.dv_y;

	if (!ZERO(dx) || !ZERO(dy)) {
	    gvp->gv_s->gv_adc.a1 = RAD2DEG*atan2(dy, dx);
	    gvp->gv_s->gv_adc.dv_a1 = (1.0 - (gvp->gv_s->gv_adc.a1 / 45.0)) * BV_MAX;
	}
    }
}


void
ged_calc_adc_a2(struct bview *gvp)
{
    if (gvp->gv_s->gv_adc.anchor_a2) {
	fastf_t dx, dy;
	point_t view_pt;

	MAT4X3PNT(view_pt, gvp->gv_model2view, gvp->gv_s->gv_adc.anchor_pt_a2);
	dx = view_pt[X] * BV_MAX - gvp->gv_s->gv_adc.dv_x;
	dy = view_pt[Y] * BV_MAX - gvp->gv_s->gv_adc.dv_y;

	if (!ZERO(dx) || !ZERO(dy)) {
	    gvp->gv_s->gv_adc.a2 = RAD2DEG*atan2(dy, dx);
	    gvp->gv_s->gv_adc.dv_a2 = (1.0 - (gvp->gv_s->gv_adc.a2 / 45.0)) * BV_MAX;
	}
    }
}


void
ged_calc_adc_dst(struct bview *gvp)
{
    if (gvp->gv_s->gv_adc.anchor_dst) {
	fastf_t dist;
	fastf_t dx, dy;
	point_t view_pt;

	MAT4X3PNT(view_pt, gvp->gv_model2view, gvp->gv_s->gv_adc.anchor_pt_dst);

	dx = view_pt[X] * BV_MAX - gvp->gv_s->gv_adc.dv_x;
	dy = view_pt[Y] * BV_MAX - gvp->gv_s->gv_adc.dv_y;
	dist = sqrt(dx * dx + dy * dy);
	gvp->gv_s->gv_adc.dst = dist * INV_BV;
	gvp->gv_s->gv_adc.dv_dist = (dist / M_SQRT1_2) - BV_MAX;
    } else
	gvp->gv_s->gv_adc.dst = (gvp->gv_s->gv_adc.dv_dist * INV_BV + 1.0) * M_SQRT1_2;
}

#ifdef GED_PLUGIN
#include "../include/plugin.h"
struct ged_cmd_impl adc_cmd_impl = {
    "adc",
    ged_adc_core,
    GED_CMD_DEFAULT
};

const struct ged_cmd adc_cmd = { &adc_cmd_impl };
const struct ged_cmd *adc_cmds[] = { &adc_cmd, NULL };

static const struct ged_plugin pinfo = { GED_API,  adc_cmds, 1 };

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
