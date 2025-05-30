/*                         E D V O L . C
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
/** @file primitives/edvol.c
 *
 */

#include "common.h"

#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "rt/geom.h"
#include "wdb.h"

#include "../edit_private.h"

#define ECMD_VOL_CSIZE		13048	/* set voxel size */
#define ECMD_VOL_FSIZE		13049	/* set VOL file dimensions */
#define ECMD_VOL_THRESH_LO	13050	/* set VOL threshold (lo) */
#define ECMD_VOL_THRESH_HI	13051	/* set VOL threshold (hi) */
#define ECMD_VOL_FNAME		13052	/* set VOL file name */

void
rt_edit_vol_set_edit_mode(struct rt_edit *s, int mode)
{
    rt_edit_set_edflag(s, mode);

    switch (mode) {
	case ECMD_VOL_FNAME:
	    break;
	case ECMD_VOL_FSIZE:
	    break;
	case ECMD_VOL_CSIZE:
	case ECMD_VOL_THRESH_LO:
	case ECMD_VOL_THRESH_HI:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
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
vol_ed(struct rt_edit *s, int arg, int UNUSED(a), int UNUSED(b), void *UNUSED(data))
{
    rt_edit_set_edflag(s, arg);

    switch (arg) {
	case ECMD_VOL_FNAME:
	    break;
	case ECMD_VOL_FSIZE:
	    break;
	case ECMD_VOL_CSIZE:
	case ECMD_VOL_THRESH_LO:
	case ECMD_VOL_THRESH_HI:
	    s->edit_mode = RT_PARAMS_EDIT_SCALE;
	    break;
    }

    // TODO - should we be calling this here?
    rt_edit_process(s);

    bu_clbk_t f = NULL;
    void *d = NULL;
    int flag = 1;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_EAXES_POS, BU_CLBK_DURING);
    if (f)
	(*f)(0, NULL, d, &flag);
}

struct rt_edit_menu_item vol_menu[] = {
    {"VOL MENU", NULL, 0 },
    {"File Name", vol_ed, ECMD_VOL_FNAME },
    {"File Size (X Y Z)", vol_ed, ECMD_VOL_FSIZE },
    {"Voxel Size (X Y Z)", vol_ed, ECMD_VOL_CSIZE },
    {"Threshold (low)", vol_ed, ECMD_VOL_THRESH_LO },
    {"Threshold (hi)", vol_ed, ECMD_VOL_THRESH_HI },
    { "", NULL, 0 }
};

struct rt_edit_menu_item *
rt_edit_vol_menu_item(const struct bn_tol *UNUSED(tol))
{
    return vol_menu;
}

/* set voxel size */
void
ecmd_vol_csize(struct rt_edit *s)
{
    struct rt_vol_internal *vol = (struct rt_vol_internal *)s->es_int.idb_ptr;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_VOL_CK_MAGIC(vol);

    // Specified numerical input
    if (s->e_inpara) {
	if (s->e_inpara != 3) {
	    bu_vls_printf(s->log_str, "x, y, and z cell sizes are required\n");
	    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	    if (f)
		(*f)(0, NULL, d, NULL);
	    return;
	} else {
	    VMOVE(vol->cellsize, s->e_para);
	    return;
	}
    }

    // XY coord (usually mouse) based scaling
    if (s->es_scale > 0.0) {
	VSCALE(vol->cellsize, vol->cellsize, s->es_scale);
	s->es_scale = 0.0;
    }
}

/* set file size */
int
ecmd_vol_fsize(struct rt_edit *s)
{
    struct rt_vol_internal *vol =
	(struct rt_vol_internal *)s->es_int.idb_ptr;
    struct stat stat_buf;
    b_off_t need_size;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_VOL_CK_MAGIC(vol);

    if (s->e_inpara == 3) {
	if (stat(vol->name, &stat_buf)) {
	    bu_vls_printf(s->log_str, "Cannot get status of file %s\n", vol->name);
	    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	    if (f)
		(*f)(0, NULL, d, NULL);
	    return BRLCAD_ERROR;
	}
	need_size = s->e_para[0] * s->e_para[1] * s->e_para[2] * sizeof(unsigned char);
	if (stat_buf.st_size < need_size) {
	    bu_vls_printf(s->log_str, "File (%s) is too small, set file name first", vol->name);
	    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	    if (f)
		(*f)(0, NULL, d, NULL);
	    return BRLCAD_ERROR;
	}
	vol->xdim = s->e_para[0];
	vol->ydim = s->e_para[1];
	vol->zdim = s->e_para[2];
    } else if (s->e_inpara > 0) {
	bu_vls_printf(s->log_str, "x, y, and z file sizes are required\n");
	rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	if (f)
	    (*f)(0, NULL, d, NULL);
	return BRLCAD_ERROR;
    }

    return BRLCAD_OK;
}

int
ecmd_vol_thresh_lo(struct rt_edit *s)
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

    struct rt_vol_internal *vol =
	(struct rt_vol_internal *)s->es_int.idb_ptr;

    RT_VOL_CK_MAGIC(vol);

    size_t i = vol->lo;
    if (s->e_inpara) {
	i = s->e_para[0];
    } else if (s->es_scale > 0.0) {
	i = vol->lo * s->es_scale;
	if (i == vol->lo && s->es_scale > 1.0) {
	    i++;
	} else if (i == vol->lo && s->es_scale < 1.0) {
	    i--;
	}
    }

    if (i > 255)
	i = 255;

    vol->lo = i;

    return 0;
}

int
ecmd_vol_thresh_hi(struct rt_edit *s)
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

    struct rt_vol_internal *vol =
	(struct rt_vol_internal *)s->es_int.idb_ptr;

    RT_VOL_CK_MAGIC(vol);

    size_t i = vol->hi;
    if (s->e_inpara) {
	i = s->e_para[0];
    } else if (s->es_scale > 0.0) {
	i = vol->hi * s->es_scale;
	if (i == vol->hi && s->es_scale > 1.0) {
	    i++;
	} else if (i == vol->hi && s->es_scale < 1.0) {
	    i--;
	}
    }

    if (i > 255)
	i = 255;

    vol->hi = i;

    return 0;
}

int
ecmd_vol_fname(struct rt_edit *s)
{
    struct rt_vol_internal *vol =
	(struct rt_vol_internal *)s->es_int.idb_ptr;
    char *fname = NULL;
    struct stat stat_buf;
    b_off_t need_size;
    bu_clbk_t f = NULL;
    void *d = NULL;

    RT_VOL_CK_MAGIC(vol);

    const char *av[2] = {NULL};
    av[0] = vol->name;
    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_GET_FILENAME, BU_CLBK_DURING);
    if (f)
	(*f)(1, (const char **)av, d, &fname);

    if (fname) {
	if (stat(fname, &stat_buf)) {
	    // We were calling Tcl_SetResult here, which reset the result str, so zero out log_str
	    bu_vls_trunc(s->log_str, 0);
	    bu_vls_printf(s->log_str, "Cannot get status of file %s\n", fname);
	    f = NULL; d = NULL;
	    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	    if (f)
		(*f)(0, NULL, d, NULL);
	    return BRLCAD_ERROR;
	}
	need_size = vol->xdim * vol->ydim * vol->zdim * sizeof(unsigned char);
	if (stat_buf.st_size < need_size) {
	    // We were calling Tcl_SetResult here, which reset the result str, so zero out log_str
	    bu_vls_trunc(s->log_str, 0);
	    bu_vls_printf(s->log_str, "File (%s) is too small, adjust the file size parameters first", fname);
	    f = NULL; d = NULL;
	    rt_edit_map_clbk_get(&f, &d, s->m, ECMD_PRINT_RESULTS, BU_CLBK_DURING);
	    if (f)
		(*f)(0, NULL, d, NULL);
	    return BRLCAD_ERROR;
	}
	bu_strlcpy(vol->name, fname, RT_VOL_NAME_LEN);
    }

    return BRLCAD_OK;
}

static int
rt_edit_vol_pscale(struct rt_edit *s)
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
	case ECMD_VOL_CSIZE:
	    ecmd_vol_csize(s);
	    break;
    };

    return 0;
}

int
rt_edit_vol_edit(struct rt_edit *s)
{
    switch (s->edit_flag) {
	case RT_PARAMS_EDIT_SCALE:
	    /* scale the solid uniformly about its vertex point */
	    return edit_sscale(s);
	case RT_PARAMS_EDIT_TRANS:
	    /* translate solid */
	    edit_stra(s);
	    break;
	case RT_PARAMS_EDIT_ROT:
	    /* rot solid about vertex */
	    edit_srot(s);
	    break;
	case ECMD_VOL_FSIZE:
	    return ecmd_vol_fsize(s);
	case ECMD_VOL_THRESH_LO:
	    return ecmd_vol_thresh_lo(s);
	case ECMD_VOL_THRESH_HI:
	    return ecmd_vol_thresh_hi(s);
	case ECMD_VOL_FNAME:
	    return ecmd_vol_fname(s);
	default:
	    return rt_edit_vol_pscale(s);
    }

    return 0;
}

int
rt_edit_vol_edit_xy(
	struct rt_edit *s,
	const vect_t mousevec
	)
{
    vect_t pos_view = VINIT_ZERO;       /* Unrotated view space pos */
    struct rt_db_internal *ip = &s->es_int;
    bu_clbk_t f = NULL;
    void *d = NULL;

    switch (s->edit_flag) {
	case RT_PARAMS_EDIT_SCALE:
	case ECMD_VOL_CSIZE:
	case ECMD_VOL_THRESH_LO:
	case ECMD_VOL_THRESH_HI:
	    edit_sscale_xy(s, mousevec);
	    return 0;
	case RT_PARAMS_EDIT_TRANS:
	    edit_stra_xy(&pos_view, s, mousevec);
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
