/*                       T A B L E . C P P
 * BRL-CAD
 *
 * Copyright (c) 1989-2025 United States Government as represented by
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
/** @addtogroup librt */
/** @{ */
/** @file primitives/table.c
 *
 * Tables for the BRL-CAD Package ray-tracing library "librt".
 *
 */
/** @} */

#include "common.h"

#include <string.h>
#include <math.h>
#include "bio.h"


#include "vmath.h"
#include "rt/db4.h"
#include "nmg.h"
#include "raytrace.h"
#include "rt/geom.h"

extern "C" {

#define RT_DECLARE_INTERFACE(name) \
    extern int rt_##name##_prep(struct soltab *stp, struct rt_db_internal *ip, struct rt_i *rtip); \
    extern int rt_##name##_shot(struct soltab *stp, struct xray *rp, struct application *ap, struct seg *seghead); \
    extern int rt_##name##_piece_shot(struct rt_piecestate *psp, struct rt_piecelist *plp, double dist_corr, struct xray *rp, struct application *ap, struct seg *seghead); \
    extern void rt_##name##_piece_hitsegs(struct rt_piecestate *psp, struct seg *seghead, struct application *ap); \
    extern void rt_##name##_print(const struct soltab *stp); \
    extern void rt_##name##_norm(struct hit *hitp, struct soltab *stp, struct xray *rp); \
    extern void rt_##name##_uv(struct application *ap, struct soltab *stp, struct hit *hitp, struct uvcoord *uvp); \
    extern void rt_##name##_curve(struct curvature *cvp, struct hit *hitp, struct soltab *stp); \
    extern int rt_##name##_class(const struct soltab *, const vect_t *, const vect_t *, const struct bn_tol *); \
    extern void rt_##name##_free(struct soltab *stp); \
    extern int rt_##name##_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct bg_tess_tol *ttol, const struct bn_tol *tol, const struct bview *info); \
    extern int rt_##name##_adaptive_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct bn_tol *tol, const struct bview *v, fastf_t s_size); \
    extern void rt_##name##_vshot(struct soltab *stp[], struct xray *rp[], struct seg *segp, int n, struct application *ap); \
    extern int rt_##name##_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct bg_tess_tol *ttol, const struct bn_tol *tol); \
    extern int rt_##name##_tnurb(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct bn_tol *tol); \
    extern void rt_##name##_brep(ON_Brep **b, const struct rt_db_internal *ip, const struct bn_tol *tol); \
    extern int rt_##name##_import5(struct rt_db_internal *ip, const struct bu_external *ep, const mat_t mat, const struct db_i *dbip); \
    extern int rt_##name##_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip); \
    extern int rt_##name##_import4(struct rt_db_internal *ip, const struct bu_external *ep, const mat_t mat, const struct db_i *dbip); \
    extern int rt_##name##_export4(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip); \
    extern void rt_##name##_ifree(struct rt_db_internal *ip); \
    extern int rt_##name##_get(struct bu_vls *logstr, const struct rt_db_internal *intern, const char *attr); \
    extern int rt_##name##_adjust(struct bu_vls *logstr, struct rt_db_internal *intern, int argc, const char **argv); \
    extern int rt_##name##_describe(struct bu_vls *str, const struct rt_db_internal *ip, int verbose, double mm2local); \
    extern void rt_##name##_make(const struct rt_functab *ftp, struct rt_db_internal *intern); \
    extern int rt_##name##_xform(struct rt_db_internal *op, const mat_t mat, struct rt_db_internal *ip, int release, struct db_i *dbip); \
    extern int rt_##name##_params(struct pc_pc_set *ps, const struct rt_db_internal *ip); \
    extern int rt_##name##_bbox(struct rt_db_internal *ip, point_t *min, point_t *max, const struct bn_tol *tol); \
    extern int rt_##name##_mirror(struct rt_db_internal *ip, const plane_t *plane); \
    extern const struct bu_structparse rt_##name##_parse[]; \
    extern void rt_##name##_volume(fastf_t *vol, const struct rt_db_internal *ip); \
    extern void rt_##name##_surf_area(fastf_t *area, const struct rt_db_internal *ip); \
    extern void rt_##name##_centroid(point_t *cent, const struct rt_db_internal *ip); \
    extern int rt_##name##_oriented_bbox(struct rt_arb_internal *bbox, struct rt_db_internal *ip, const fastf_t tol); \
    extern struct rt_selection_set *rt_##name##_find_selections(const struct rt_db_internal *ip, const struct rt_selection_query *query); \
    extern struct rt_selection *rt_##name##_evaluate_selection(const struct rt_db_internal *ip, int op, const struct rt_selection *a, const struct rt_selection *b); \
    extern int rt_##name##_process_selection(struct rt_db_internal *ip, struct db_i *, const struct rt_selection *selection, const struct rt_selection_operation *op); \
    extern int rt_##name##_prep_serialize(struct soltab *stp, const struct rt_db_internal *ip, struct bu_external *external, size_t *version); \
    extern int rt_##name##_labels(struct rt_point_labels *pl, int pl_max, const mat_t xform, const struct rt_db_internal *ip, const struct bn_tol *utol); \
    extern const char *rt_##name##_keypoint(point_t *pt, const char *keystr, const mat_t xform, const struct rt_db_internal *ip, const struct bn_tol *tol); \
    extern int rt_##name##_mat(struct rt_db_internal *op, const mat_t mat, const struct rt_db_internal *ip); \
    extern int rt_##name##_perturb(struct rt_db_internal **oip, const struct rt_db_internal *ip, int planar_only, fastf_t factor); \
    extern int rt_##name##_scene_obj(struct bv_scene_obj *vhead, struct directory *dp, struct db_i *dbip, const struct bg_tess_tol *ttol, const struct bn_tol *tol, const struct bview *info) \

RT_DECLARE_INTERFACE(tor);
RT_DECLARE_INTERFACE(tgc);
RT_DECLARE_INTERFACE(ell);
RT_DECLARE_INTERFACE(arb);
RT_DECLARE_INTERFACE(ars);
RT_DECLARE_INTERFACE(hlf);
RT_DECLARE_INTERFACE(rec);
RT_DECLARE_INTERFACE(pg);
RT_DECLARE_INTERFACE(nurb);
RT_DECLARE_INTERFACE(sph);
RT_DECLARE_INTERFACE(ebm);
RT_DECLARE_INTERFACE(vol);
RT_DECLARE_INTERFACE(arbn);
RT_DECLARE_INTERFACE(pipe);
RT_DECLARE_INTERFACE(part);
RT_DECLARE_INTERFACE(nmg);
RT_DECLARE_INTERFACE(rpc);
RT_DECLARE_INTERFACE(rhc);
RT_DECLARE_INTERFACE(epa);
RT_DECLARE_INTERFACE(ehy);
RT_DECLARE_INTERFACE(eto);
RT_DECLARE_INTERFACE(grp);
RT_DECLARE_INTERFACE(hf);
RT_DECLARE_INTERFACE(dsp);
RT_DECLARE_INTERFACE(sketch);
RT_DECLARE_INTERFACE(annot);
RT_DECLARE_INTERFACE(extrude);
RT_DECLARE_INTERFACE(submodel);
RT_DECLARE_INTERFACE(cline);
RT_DECLARE_INTERFACE(bot);
RT_DECLARE_INTERFACE(superell);
RT_DECLARE_INTERFACE(metaball);
RT_DECLARE_INTERFACE(hyp);
RT_DECLARE_INTERFACE(revolve);
RT_DECLARE_INTERFACE(constraint);
RT_DECLARE_INTERFACE(material);
/* RT_DECLARE_INTERFACE(binunif); */
RT_DECLARE_INTERFACE(pnts);
RT_DECLARE_INTERFACE(hrt);
RT_DECLARE_INTERFACE(datum);
RT_DECLARE_INTERFACE(brep);
RT_DECLARE_INTERFACE(joint);
RT_DECLARE_INTERFACE(script);


/* generics for object manipulation, in generic.c */
extern int rt_generic_get(struct bu_vls *, const struct rt_db_internal *, const char *);
extern int rt_generic_adjust(struct bu_vls *, struct rt_db_internal *, int, const char **);
extern int rt_generic_form(struct bu_vls *, const struct rt_functab *);
extern void rt_generic_make(const struct rt_functab *, struct rt_db_internal *);
extern int rt_generic_xform(struct rt_db_internal *, const mat_t, struct rt_db_internal *, int, struct db_i *);
extern int rt_generic_scene_obj(struct bv_scene_obj *s, struct directory *dp, struct db_i *dbip, const struct bg_tess_tol *ttol, const struct bn_tol *tol, const struct bview *v);

/* from db5_bin.c */
extern int rt_binunif_import5(struct rt_db_internal * ip, const struct bu_external *ep, const mat_t mat, const struct db_i *dbip, struct resource *resp);
extern int rt_binunif_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip, struct resource *resp);
extern void rt_binunif_ifree(struct rt_db_internal *ip);
extern int rt_binunif_describe(struct bu_vls *str, const struct rt_db_internal *ip, int verbose, double mm2local);
extern void rt_binunif_make(const struct rt_functab *ftp, struct rt_db_internal *intern);
extern int rt_binunif_get(struct bu_vls *logstr, const struct rt_db_internal *intern, const char *attr);
extern int rt_binunif_adjust(struct bu_vls *logstr, struct rt_db_internal *intern, int argc, const char **argv);

/* from comb/comb.c and comb/db_comb.c */
extern int rt_comb_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip, struct resource *resp);
extern int rt_comb_import5(struct rt_db_internal *ip, const struct bu_external *ep, const mat_t mat, const struct db_i *dbip, struct resource *resp);
extern int rt_comb_get(struct bu_vls *logstr, const struct rt_db_internal *intern, const char *item);
extern int rt_comb_adjust(struct bu_vls *logstr, struct rt_db_internal *intern, int argc, const char **argv);
extern int rt_comb_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern void rt_comb_make(const struct rt_functab *ftp, struct rt_db_internal *intern);
extern void rt_comb_ifree(struct rt_db_internal *ip);
extern int rt_comb_mat(struct rt_db_internal *op, const mat_t mat, const struct rt_db_internal *ip);
extern int rt_comb_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct bg_tess_tol *ttol, const struct bn_tol *tol);

extern int rt_annot_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern int rt_bot_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern int rt_cline_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern int rt_ebm_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern int rt_extrude_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern int rt_metaball_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern int rt_script_form(struct bu_vls *logstr, const struct rt_functab *ftp);
extern int rt_sketch_form(struct bu_vls *logstr, const struct rt_functab *ftp);


const struct rt_functab OBJ[] = {
    {
	/* 0: unused, for sanity checking. */
	RT_FUNCTAB_MAGIC, "ID_NULL", "NULL",
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
    },

    {
	/* 1 */
	RT_FUNCTAB_MAGIC, "ID_TOR", "tor",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_tor_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_tor_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_tor_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_tor_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_tor_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_tor_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_tor_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_tor_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_tor_adaptive_plot),
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_tor_vshot),
	RTFUNCTAB_FUNC_TESS_CAST(rt_tor_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_tor_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_tor_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_tor_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_tor_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_tor_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_tor_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_tor_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_tor_parse,
	sizeof(struct rt_tor_internal),
	RT_TOR_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_tor_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_tor_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_tor_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_tor_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_tor_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_tor_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_tor_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_tor_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 2 */
	RT_FUNCTAB_MAGIC, "ID_TGC", "tgc",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_tgc_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_tgc_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_tgc_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_tgc_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_tgc_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_tgc_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_tgc_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_tgc_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_tgc_adaptive_plot),
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_tgc_vshot),
	RTFUNCTAB_FUNC_TESS_CAST(rt_tgc_tess),
	RTFUNCTAB_FUNC_TNURB_CAST(rt_tgc_tnurb),
	RTFUNCTAB_FUNC_BREP_CAST(rt_tgc_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_tgc_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_tgc_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_tgc_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_tgc_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_tgc_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_tgc_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_tgc_parse,
	sizeof(struct rt_tgc_internal),
	RT_TGC_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_tgc_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_tgc_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_tgc_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_tgc_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_tgc_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_tgc_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_tgc_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_tgc_mat),
	RTFUNCTAB_FUNC_PERTURB_CAST(rt_tgc_perturb), /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 3 */
	RT_FUNCTAB_MAGIC, "ID_ELL", "ell",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_ell_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_ell_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_ell_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_ell_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_ell_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_ell_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_ell_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_ell_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_ell_adaptive_plot),
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_ell_vshot),
	RTFUNCTAB_FUNC_TESS_CAST(rt_ell_tess),
	RTFUNCTAB_FUNC_TNURB_CAST(rt_ell_tnurb),
	RTFUNCTAB_FUNC_BREP_CAST(rt_ell_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_ell_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_ell_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_ell_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_ell_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_ell_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_ell_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_ell_parse,
	sizeof(struct rt_ell_internal),
	RT_ELL_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_ell_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_ell_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_ell_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_ell_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_ell_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_ell_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_ell_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_ell_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 4 */
	RT_FUNCTAB_MAGIC, "ID_ARB8", "arb8",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_arb_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_arb_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_arb_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_arb_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_arb_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_arb_curve),
	RTFUNCTAB_FUNC_CLASS_CAST(rt_arb_class),
	RTFUNCTAB_FUNC_FREE_CAST(rt_arb_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_arb_plot),
	NULL, /* adaptive_plot */
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_arb_vshot),
	RTFUNCTAB_FUNC_TESS_CAST(rt_arb_tess),
	RTFUNCTAB_FUNC_TNURB_CAST(rt_arb_tnurb),
	RTFUNCTAB_FUNC_BREP_CAST(rt_arb_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_arb_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_arb_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_arb_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_arb_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_arb_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_arb_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_arb_parse,
	sizeof(struct rt_arb_internal),
	RT_ARB_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_arb_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_arb_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_arb_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_arb_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_arb_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_arb_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_arb_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_arb_mat),
	RTFUNCTAB_FUNC_PERTURB_CAST(rt_arb_perturb), /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 5 */
	RT_FUNCTAB_MAGIC, "ID_ARS", "ars",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_ars_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_bot_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_bot_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_bot_norm),
	NULL, /* piece_shot */
	NULL, /* piece hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_bot_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_bot_curve),
	NULL, /* classify */
	RTFUNCTAB_FUNC_FREE_CAST(rt_bot_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_ars_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_ars_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_ars_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_ars_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_ars_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_ars_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_ars_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_ars_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_ars_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_ars_internal),
	RT_ARS_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_ars_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_ars_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_ars_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_ars_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_ars_labels), /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_ars_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 6 */
	RT_FUNCTAB_MAGIC, "ID_HALF", "half",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_hlf_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_hlf_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_hlf_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_hlf_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_hlf_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_hlf_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_hlf_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_hlf_plot),
	NULL, /* adaptive_plot */
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_hlf_vshot),
	RTFUNCTAB_FUNC_TESS_CAST(rt_hlf_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_hlf_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_hlf_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_hlf_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_hlf_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_hlf_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_hlf_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_hlf_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_hlf_xform),
	rt_hlf_parse,
	sizeof(struct rt_half_internal),
	RT_HALF_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_hlf_params),
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_hlf_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_hlf_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 7 */
	RT_FUNCTAB_MAGIC, "ID_REC", "rec",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_rec_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_rec_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_rec_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_rec_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_rec_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_rec_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_rec_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_tgc_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_tgc_adaptive_plot),
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_rec_vshot),
	RTFUNCTAB_FUNC_TESS_CAST(rt_tgc_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_tgc_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_tgc_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_tgc_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_tgc_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_tgc_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_tgc_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_tgc_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_tgc_parse,
	sizeof(struct rt_tgc_internal),
	RT_TGC_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_rec_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_rec_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_tgc_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_tgc_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_tgc_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_tgc_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_tgc_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_tgc_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 8 */
	RT_FUNCTAB_MAGIC, "ID_POLY", "poly",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_pg_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_pg_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_pg_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_pg_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_pg_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_pg_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_pg_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_pg_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_pg_tess),
	NULL, /* tnurb */
	NULL, /* brep */
	NULL, /* import5 */
	NULL, /* export5 */
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_pg_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_pg_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_pg_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_pg_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_pg_internal),
	RT_PG_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_pg_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_pg_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_pg_keypoint), /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	NULL  /* scene_obj */
    },

    {
	/* 9 */
	RT_FUNCTAB_MAGIC, "ID_BSPLINE", "bspline",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_nurb_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_nurb_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_nurb_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_nurb_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_nurb_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_nurb_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_nurb_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_nurb_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_nurb_tess),
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_nurb_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_nurb_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_nurb_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_nurb_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_nurb_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_nurb_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_nurb_internal),
	RT_NURB_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_nurb_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_nurb_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_nurb_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_nurb_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_nurb_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 10 */
	RT_FUNCTAB_MAGIC, "ID_SPH", "sph",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_sph_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_sph_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_sph_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_sph_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_sph_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_sph_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_sph_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_ell_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_ell_adaptive_plot),
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_sph_vshot),
	RTFUNCTAB_FUNC_TESS_CAST(rt_ell_tess),
	RTFUNCTAB_FUNC_TNURB_CAST(rt_ell_tnurb),
	RTFUNCTAB_FUNC_BREP_CAST(rt_ell_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_ell_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_ell_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_ell_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_ell_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_ell_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_ell_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_ell_parse,
	sizeof(struct rt_ell_internal),
	RT_ELL_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_sph_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_ell_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_ell_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_ell_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_ell_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_ell_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_ell_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_ell_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 11 */
	RT_FUNCTAB_MAGIC, "ID_NMG", "nmg",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_nmg_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_nmg_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_nmg_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_nmg_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_nmg_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_nmg_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_nmg_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_nmg_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_nmg_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_nmg_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_nmg_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_nmg_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_nmg_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_nmg_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_nmg_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_nmg_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct model),
	NMG_MODEL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_nmg_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_nmg_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_nmg_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_nmg_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_nmg_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_nmg_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_nmg_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_nmg_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_nmg_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 12 */
	RT_FUNCTAB_MAGIC, "ID_EBM", "ebm",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_ebm_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_ebm_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_ebm_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_ebm_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_ebm_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_ebm_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_ebm_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_ebm_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_ebm_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_ebm_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_ebm_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_ebm_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_ebm_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_ebm_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_ebm_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_ebm_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_ebm_parse,
	sizeof(struct rt_ebm_internal),
	RT_EBM_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_ebm_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_ebm_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_ebm_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_ebm_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_ebm_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_ebm_bbox),
	NULL, /* volume */
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_ebm_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_ebm_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_ebm_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_ebm_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 13 */
	RT_FUNCTAB_MAGIC, "ID_VOL", "vol",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_vol_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_vol_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_vol_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_vol_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_vol_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_vol_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_vol_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_vol_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_vol_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_vol_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_vol_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_vol_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_vol_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_vol_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_vol_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_vol_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_vol_parse,
	sizeof(struct rt_vol_internal),
	RT_VOL_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_vol_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_vol_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_vol_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_vol_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_vol_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_vol_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_vol_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 14 */
	RT_FUNCTAB_MAGIC, "ID_ARBN", "arbn",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_arbn_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_arbn_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_arbn_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_arbn_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_arbn_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_arbn_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_arbn_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_arbn_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_arbn_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_arbn_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_arbn_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_arbn_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_arbn_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_arbn_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_arbn_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_arbn_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_arbn_internal),
	RT_ARBN_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_arbn_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_arbn_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_arbn_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_arbn_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_arbn_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_arbn_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_arbn_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_arbn_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_arbn_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_arbn_mat),
	RTFUNCTAB_FUNC_PERTURB_CAST(rt_arbn_perturb), /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 15 */
	RT_FUNCTAB_MAGIC, "ID_PIPE", "pipe",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_pipe_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_pipe_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_pipe_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_pipe_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_pipe_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_pipe_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_pipe_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_pipe_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_pipe_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_pipe_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_pipe_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_pipe_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_pipe_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_pipe_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_pipe_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_pipe_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_pipe_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_pipe_internal),
	RT_PIPE_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_pipe_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_pipe_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_pipe_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_pipe_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_pipe_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_pipe_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_pipe_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_pipe_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_pipe_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 16 */
	RT_FUNCTAB_MAGIC, "ID_PARTICLE", "part",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_part_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_part_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_part_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_part_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_part_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_part_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_part_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_part_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_part_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_part_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_part_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_part_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_part_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_part_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_part_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_part_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_part_parse,
	sizeof(struct rt_part_internal),
	RT_PART_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_part_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_part_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_part_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_part_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_part_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_part_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_part_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_part_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 17 */
	RT_FUNCTAB_MAGIC, "ID_RPC", "rpc",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_rpc_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_rpc_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_rpc_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_rpc_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_rpc_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_rpc_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_rpc_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_rpc_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_rpc_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_rpc_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_rpc_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_rpc_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_rpc_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_rpc_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_rpc_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_rpc_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_rpc_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_rpc_parse,
	sizeof(struct rt_rpc_internal),
	RT_RPC_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_rpc_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_rpc_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_rpc_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_rpc_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_rpc_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_rpc_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_rpc_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_rpc_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_rpc_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 18 */
	RT_FUNCTAB_MAGIC, "ID_RHC", "rhc",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_rhc_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_rhc_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_rhc_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_rhc_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_rhc_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_rhc_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_rhc_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_rhc_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_rhc_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_rhc_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_rhc_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_rhc_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_rhc_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_rhc_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_rhc_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_rhc_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_rhc_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_rhc_parse,
	sizeof(struct rt_rhc_internal),
	RT_RHC_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_rhc_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_rhc_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_rhc_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_rhc_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_rhc_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_rhc_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_rhc_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_rhc_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_rhc_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 19 */
	RT_FUNCTAB_MAGIC, "ID_EPA", "epa",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_epa_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_epa_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_epa_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_epa_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_epa_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_epa_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_epa_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_epa_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_epa_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_epa_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_epa_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_epa_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_epa_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_epa_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_epa_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_epa_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_epa_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_epa_parse,
	sizeof(struct rt_epa_internal),
	RT_EPA_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_epa_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_epa_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_epa_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_epa_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_epa_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_epa_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_epa_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_epa_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_epa_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 20 */
	RT_FUNCTAB_MAGIC, "ID_EHY", "ehy",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_ehy_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_ehy_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_ehy_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_ehy_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_ehy_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_ehy_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_ehy_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_ehy_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_ehy_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_ehy_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_ehy_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_ehy_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_ehy_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_ehy_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_ehy_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_ehy_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_ehy_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_ehy_parse,
	sizeof(struct rt_ehy_internal),
	RT_EHY_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_ehy_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_ehy_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_ehy_bbox),
	NULL, /* volume */
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_ehy_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_ehy_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_epa_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_ehy_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_ehy_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 21 */
	RT_FUNCTAB_MAGIC, "ID_ETO", "eto",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_eto_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_eto_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_eto_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_eto_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_eto_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_eto_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_eto_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_eto_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_eto_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_eto_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_eto_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_eto_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_eto_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_eto_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_eto_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_eto_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_eto_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_eto_parse,
	sizeof(struct rt_eto_internal),
	RT_ETO_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_eto_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_eto_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_eto_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_eto_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_eto_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_eto_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_eto_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_eto_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_eto_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 22 */
	RT_FUNCTAB_MAGIC, "ID_GRIP", "grip",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_grp_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_grp_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_grp_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_grp_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_grp_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_grp_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_grp_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_grp_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_grp_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_grp_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_grp_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_grp_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_grp_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_grp_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_grp_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_grp_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_grp_parse,
	sizeof(struct rt_grip_internal),
	RT_GRIP_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_grp_params),
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_grp_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_grp_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 23 -- XXX unimplemented */
	RT_FUNCTAB_MAGIC, "ID_JOINT", "joint",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_joint_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_joint_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_joint_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_joint_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_joint_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_joint_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_joint_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_joint_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_joint_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_joint_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_joint_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_joint_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_joint_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_joint_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_joint_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_joint_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_joint_parse,
	sizeof(struct rt_joint_internal),
	RT_JOINT_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_joint_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_joint_params),
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	RTFUNCTAB_FUNC_FIND_SELECTIONS_CAST(rt_joint_find_selections),
	NULL, /* evaluate_selections */
	RTFUNCTAB_FUNC_PROCESS_SELECTION_CAST(rt_joint_process_selection),
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_joint_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 24 */
	RT_FUNCTAB_MAGIC, "ID_HF", "hf",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_hf_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_hf_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_hf_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_hf_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_hf_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_hf_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_hf_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_hf_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_hf_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_hf_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_hf_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_hf_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_hf_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_hf_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_hf_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_hf_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_hf_parse,
	sizeof(struct rt_hf_internal),
	RT_HF_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_hf_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_hf_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_hf_keypoint), /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 25 Displacement Map (alt. height field) */
	RT_FUNCTAB_MAGIC, "ID_DSP", "dsp",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_dsp_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_dsp_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_dsp_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_dsp_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_dsp_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_dsp_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_dsp_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_dsp_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_dsp_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_dsp_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_dsp_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_dsp_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_dsp_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_dsp_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_dsp_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_dsp_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_dsp_parse,
	sizeof(struct rt_dsp_internal),
	RT_DSP_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_dsp_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_dsp_adjust),
	NULL, /* form */
	RTFUNCTAB_FUNC_MAKE_CAST(rt_dsp_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_dsp_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_dsp_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_dsp_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_dsp_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 26 2D sketch */
	RT_FUNCTAB_MAGIC, "ID_SKETCH", "sketch",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_sketch_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_sketch_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_sketch_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_sketch_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_sketch_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_sketch_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_sketch_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_sketch_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_sketch_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_sketch_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_sketch_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_sketch_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_sketch_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_sketch_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_sketch_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_sketch_internal),
	RT_SKETCH_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_sketch_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_sketch_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_sketch_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_sketch_params),
	NULL, /* bbox */
	NULL, /* volume */
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_sketch_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_sketch_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_sketch_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_sketch_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 27 Solid of extrusion */
	RT_FUNCTAB_MAGIC, "ID_EXTRUDE", "extrude",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_extrude_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_extrude_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_extrude_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_extrude_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_extrude_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_extrude_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_extrude_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_extrude_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_extrude_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_extrude_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_extrude_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_extrude_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_extrude_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_extrude_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_extrude_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_extrude_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_extrude_xform),
	NULL, /* parse */
	sizeof(struct rt_extrude_internal),
	RT_EXTRUDE_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_extrude_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_extrude_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_extrude_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_extrude_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_extrude_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_extrude_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_extrude_volume),
	NULL, /* surf_area */
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_extrude_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_extrude_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_extrude_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 28 Instanced submodel */
	RT_FUNCTAB_MAGIC, "ID_SUBMODEL", "submodel",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_submodel_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_submodel_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_submodel_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_submodel_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_submodel_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_submodel_curve),
	NULL, /* classify */
	RTFUNCTAB_FUNC_FREE_CAST(rt_submodel_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_submodel_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_submodel_tess),
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_submodel_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_submodel_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_submodel_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_submodel_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_submodel_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_submodel_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_submodel_parse,
	sizeof(struct rt_submodel_internal),
	RT_SUBMODEL_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_submodel_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_submodel_params),
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_submodel_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 29 Fastgen cline solid */
	RT_FUNCTAB_MAGIC, "ID_CLINE", "cline",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_cline_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_cline_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_cline_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_cline_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_cline_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_cline_curve),
	NULL, /* classify */
	RTFUNCTAB_FUNC_FREE_CAST(rt_cline_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_cline_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_cline_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_cline_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_cline_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_cline_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_cline_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_cline_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_cline_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_cline_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_cline_parse,
	sizeof(struct rt_cline_internal),
	RT_CLINE_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_cline_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_cline_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_cline_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_cline_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_cline_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_cline_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_cline_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_cline_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 30 Bag o' Triangles */
	RT_FUNCTAB_MAGIC, "ID_BOT", "bot",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_bot_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_bot_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_bot_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_bot_norm),
	NULL, /* piece shot */
	NULL, /* piece hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_bot_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_bot_curve),
	NULL, /* classify */
	RTFUNCTAB_FUNC_FREE_CAST(rt_bot_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_bot_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_bot_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_bot_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_bot_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_bot_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_bot_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_bot_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_bot_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_bot_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_bot_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_bot_xform),
	NULL, /* parse */
	sizeof(struct rt_bot_internal),
	RT_BOT_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_bot_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_bot_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_bot_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_bot_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_bot_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_bot_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_bot_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_bot_centroid),
	RTFUNCTAB_FUNC_ORIENTED_BBOX_CAST(rt_bot_oriented_bbox),
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_bot_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_bot_mat),
	NULL, /* perturb */
	NULL   /* scene_obj */
    },

    {
	/* 31 combination objects (should not be in this table) */
	RT_FUNCTAB_MAGIC, "ID_COMBINATION", "comb",
	0,
	NULL, /* prep */
	NULL, /* shot */
	NULL, /* print */
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* classify */
	NULL, /* free */
	NULL, /* plot */
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_comb_tess),
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_comb_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_comb_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_comb_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_comb_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_comb_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_comb_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	0,
	0,
	RTFUNCTAB_FUNC_GET_CAST(rt_comb_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_comb_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_comb_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_comb_make),
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_comb_mat),
	NULL, /* perturb */
	NULL   /* scene_obj */
    },

    {
	/* 32 available placeholder to not offset latter table indices
	 * (was ID_BINEXPM)
	 */
	RT_FUNCTAB_MAGIC, "ID_UNUSED1", "UNUSED1",
	0, /* ft_use_rpp */
	NULL, /* prep */
	NULL, /* shot */
	NULL, /* print */
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* classify */
	NULL, /* free */
	NULL, /* plot */
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	NULL, /* import5 */
	NULL, /* export5 */
	NULL, /* import4 */
	NULL, /* export4 */
	NULL, /* ifree */
	NULL, /* describe */
	NULL, /* xform */
	NULL, /* parse */
	0, /* sizeof(internal) */
	0, /* magic */
	NULL, /* get */
	NULL, /* adjust */
	NULL, /* form */
	NULL, /* make */
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	NULL  /* scene_obj */
    },

    {
	/* 33 */
	RT_FUNCTAB_MAGIC, "ID_BINUNIF", "binunif",
	0,
	NULL, /* prep */
	NULL, /* shot */
	NULL, /* print */
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* classify */
	NULL, /* free */
	NULL, /* plot */
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	NULL, /* import5 */
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_binunif_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_binunif_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_binunif_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	0,
	0,
	RTFUNCTAB_FUNC_GET_CAST(rt_binunif_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_binunif_adjust),
	NULL, /* form */
	RTFUNCTAB_FUNC_MAKE_CAST(rt_binunif_make),
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	NULL  /* scene_obj */
    },

    {
	/* 34 available placeholder to not offset latter table indices
	 * (was ID_BINMIME)
	 */
	RT_FUNCTAB_MAGIC, "ID_UNUSED2", "UNUSED2",
	0, /* ft_use_rpp */
	NULL, /* prep */
	NULL, /* shot */
	NULL, /* print */
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* classify */
	NULL, /* free */
	NULL, /* plot */
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	NULL, /* import5 */
	NULL, /* export5 */
	NULL, /* import4 */
	NULL, /* export4 */
	NULL, /* ifree */
	NULL, /* describe */
	NULL, /* xform */
	NULL, /* parse */
	0, /* sizeof(internal) */
	0, /* magic */
	NULL, /* get */
	NULL, /* adjust */
	NULL, /* form */
	NULL, /* make */
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	NULL  /* scene_obj */
    },

    {
	/* 35 (but "should" be 31) Superquadratic Ellipsoid */
	RT_FUNCTAB_MAGIC, "ID_SUPERELL", "superell",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_superell_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_superell_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_superell_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_superell_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_superell_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_superell_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_superell_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_superell_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_superell_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_superell_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_superell_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_superell_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_superell_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_superell_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_superell_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_superell_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_superell_parse,
	sizeof(struct rt_superell_internal),
	RT_SUPERELL_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_superell_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_superell_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_superell_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_superell_surf_area),
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_superell_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_superell_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_superell_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 36 (but "should" be 32) Metaball */
	RT_FUNCTAB_MAGIC, "ID_METABALL", "metaball",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_metaball_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_metaball_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_metaball_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_metaball_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_metaball_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_metaball_curve),
	NULL, /* classify */
	RTFUNCTAB_FUNC_FREE_CAST(rt_metaball_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_metaball_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_metaball_tess),
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_metaball_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_metaball_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_metaball_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_metaball_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_metaball_internal),
	RT_METABALL_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_metaball_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_metaball_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_metaball_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_metaball_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_metaball_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_metaball_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_metaball_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 37 */
	RT_FUNCTAB_MAGIC, "ID_BREP", "brep",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_brep_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_brep_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_brep_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_brep_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_brep_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_brep_curve),
	NULL, /* classify */
	RTFUNCTAB_FUNC_FREE_CAST(rt_brep_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_brep_plot),
	RTFUNCTAB_FUNC_ADAPTIVE_PLOT_CAST(rt_brep_adaptive_plot),
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_brep_tess),
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_brep_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_brep_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_brep_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_brep_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_brep_internal),
	RT_BREP_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_brep_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_brep_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_brep_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_brep_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_brep_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	RTFUNCTAB_FUNC_FIND_SELECTIONS_CAST(rt_brep_find_selections),
	NULL, /* evaluate_selection */
	RTFUNCTAB_FUNC_PROCESS_SELECTION_CAST(rt_brep_process_selection),
        RTFUNCTAB_FUNC_PREP_SERIALIZE_CAST(rt_brep_prep_serialize),
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_brep_mat),
	NULL, /* perturb */
	NULL   /* scene_obj */
    },

    {
	/* 38 (but "should" be 34) Hyperboloid */
	RT_FUNCTAB_MAGIC, "ID_HYP", "hyp",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_hyp_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_hyp_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_hyp_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_hyp_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_hyp_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_hyp_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_hyp_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_hyp_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_hyp_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_hyp_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_hyp_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_hyp_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_hyp_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_hyp_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_hyp_parse,
	sizeof(struct rt_hyp_internal),
	RT_HYP_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_hyp_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_hyp_bbox),
	RTFUNCTAB_FUNC_VOLUME_CAST(rt_hyp_volume),
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_hyp_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_hyp_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	RTFUNCTAB_FUNC_LABELS_CAST(rt_hyp_labels), /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_hyp_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_hyp_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 39 */
	RT_FUNCTAB_MAGIC, "ID_CONSTRAINT", "constrnt",
	0, /* ft_use_rpp */
	NULL, /* prep */
	NULL, /* shot */
	NULL, /* print */
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* classify */
	NULL, /* free */
	NULL, /* plot */
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_constraint_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_constraint_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_constraint_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_constraint_describe),
	NULL, /* xform */
	rt_constraint_parse,
	sizeof(struct rt_constraint_internal),
	RT_CONSTRAINT_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	NULL, /* form */
	RTFUNCTAB_FUNC_MAKE_CAST(rt_constraint_make),
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 40 */
	RT_FUNCTAB_MAGIC, "ID_REVOLVE", "revolve",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_revolve_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_revolve_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_revolve_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_revolve_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_revolve_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_revolve_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_revolve_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_revolve_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_revolve_tess),
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_revolve_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_revolve_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_revolve_export5),
	NULL,
	NULL,
	RTFUNCTAB_FUNC_IFREE_CAST(rt_revolve_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_revolve_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_revolve_xform),
	rt_revolve_parse,
	sizeof(struct rt_revolve_internal),
	RT_REVOLVE_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	RTFUNCTAB_FUNC_MAKE_CAST(rt_revolve_make),
	NULL, /* params */
	RTFUNCTAB_FUNC_BBOX_CAST(rt_revolve_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_revolve_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 41 */
	RT_FUNCTAB_MAGIC, "ID_PNTS", "pnts",
	0,
	RTFUNCTAB_FUNC_PREP_CAST(rt_pnts_prep),
	NULL, /* shot */
	RTFUNCTAB_FUNC_PRINT_CAST(rt_pnts_print),
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* class */
	NULL, /* free */
	RTFUNCTAB_FUNC_PLOT_CAST(rt_pnts_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	RTFUNCTAB_FUNC_BREP_CAST(rt_pnts_brep),
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_pnts_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_pnts_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_pnts_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_pnts_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_pnts_internal),
	RT_PNTS_INTERNAL_MAGIC,
	NULL, /* get */
	NULL, /* adjust */
	NULL, /* form */
	NULL, /* make */
	NULL, /* params */
	RTFUNCTAB_FUNC_BBOX_CAST(rt_pnts_bbox),
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_pnts_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 42 */
	RT_FUNCTAB_MAGIC, "ID_ANNOT", "annot",
	0, /* ft_use_rpp */
	RTFUNCTAB_FUNC_PREP_CAST(rt_annot_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_annot_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_annot_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_annot_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_annot_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_annot_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_annot_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_annot_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_annot_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_annot_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_annot_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_annot_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_annot_internal),
	RT_ANNOT_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_annot_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_annot_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_annot_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_annot_params),
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_annot_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_annot_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },

    {
	/* 43 */
	RT_FUNCTAB_MAGIC, "ID_HRT", "hrt",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_hrt_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_hrt_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_hrt_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_hrt_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_hrt_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_hrt_plot),
	NULL, /* adaptive_plot */
	RTFUNCTAB_FUNC_VSHOT_CAST(rt_hrt_vshot),
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_hrt_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_hrt_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_hrt_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_hrt_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	rt_hrt_parse,
	sizeof(struct rt_hrt_internal),
	RT_HRT_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_hrt_params),
	RTFUNCTAB_FUNC_BBOX_CAST(rt_hrt_bbox),
	NULL, /* volume */
	RTFUNCTAB_FUNC_SURF_AREA_CAST(rt_hrt_surf_area),
	RTFUNCTAB_FUNC_CENTROID_CAST(rt_hrt_centroid),
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_hrt_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },


    {
	/* 44 */
	RT_FUNCTAB_MAGIC, "ID_DATUM", "datum",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_datum_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_datum_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_datum_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_datum_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_datum_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_datum_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_datum_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_datum_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	RTFUNCTAB_FUNC_TESS_CAST(rt_datum_tess),
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_datum_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_datum_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_datum_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_datum_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_datum_internal),
	RT_DATUM_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_generic_get),
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_generic_adjust),
	RTFUNCTAB_FUNC_FORM_CAST(rt_generic_form),
	NULL, /* make */
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	RTFUNCTAB_FUNC_KEYPOINT_CAST(rt_datum_keypoint), /* keypoint */
	RTFUNCTAB_FUNC_MAT_CAST(rt_datum_mat),
	NULL, /* perturb */
	RTFUNCTAB_FUNC_SCENE_OBJ_CAST(rt_generic_scene_obj)
    },


    {
    /* 45 */
    RT_FUNCTAB_MAGIC, "ID_SCRIPT", "script",
	1,
	RTFUNCTAB_FUNC_PREP_CAST(rt_script_prep),
	RTFUNCTAB_FUNC_SHOT_CAST(rt_script_shot),
	RTFUNCTAB_FUNC_PRINT_CAST(rt_script_print),
	RTFUNCTAB_FUNC_NORM_CAST(rt_script_norm),
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	RTFUNCTAB_FUNC_UV_CAST(rt_script_uv),
	RTFUNCTAB_FUNC_CURVE_CAST(rt_script_curve),
	NULL, /* class */
	RTFUNCTAB_FUNC_FREE_CAST(rt_script_free),
	RTFUNCTAB_FUNC_PLOT_CAST(rt_script_plot),
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_script_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_script_export5),
	RTFUNCTAB_FUNC_IMPORT4_CAST(rt_script_import4),
	RTFUNCTAB_FUNC_EXPORT4_CAST(rt_script_export4),
	RTFUNCTAB_FUNC_IFREE_CAST(rt_script_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_script_describe),
	RTFUNCTAB_FUNC_XFORM_CAST(rt_generic_xform),
	NULL, /* parse */
	sizeof(struct rt_script_internal),
	RT_SCRIPT_INTERNAL_MAGIC,
	RTFUNCTAB_FUNC_GET_CAST(rt_script_get), 
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_script_adjust), 
	RTFUNCTAB_FUNC_FORM_CAST(rt_script_form), 
	RTFUNCTAB_FUNC_MAKE_CAST(rt_script_make),
	RTFUNCTAB_FUNC_PARAMS_CAST(rt_script_params), 
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	NULL  /* scene_obj */
    },

    {
	/* 46 */
	RT_FUNCTAB_MAGIC, "ID_MATERIAL", "material",
	0, /* ft_use_rpp */
	NULL, /* prep */
	NULL, /* shot */
	NULL, /* print */
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* classify */
	NULL, /* free */
	NULL, /* plot */
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	RTFUNCTAB_FUNC_IMPORT5_CAST(rt_material_import5),
	RTFUNCTAB_FUNC_EXPORT5_CAST(rt_material_export5),
	NULL, /* import4 */
	NULL, /* export4 */
	RTFUNCTAB_FUNC_IFREE_CAST(rt_material_ifree),
	RTFUNCTAB_FUNC_DESCRIBE_CAST(rt_material_describe),
	NULL, /* xform */
	NULL, /* parse */
	0, /* sizeof(internal) */
	RT_MATERIAL_MAGIC,
	NULL, /* get */
	RTFUNCTAB_FUNC_ADJUST_CAST(rt_material_adjust),
	NULL, /* form */
	RTFUNCTAB_FUNC_MAKE_CAST(rt_material_make),
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	NULL  /* scene_obj */
    },

    {
	/* this entry for sanity only */
	0L, ">ID_MAXIMUM", ">id_max",
	0, /* ft_use_rpp */
	NULL, /* prep */
	NULL, /* shot */
	NULL, /* print */
	NULL, /* norm */
	NULL, /* piece_shot */
	NULL, /* piece_hitsegs */
	NULL, /* uv */
	NULL, /* curve */
	NULL, /* classify */
	NULL, /* free */
	NULL, /* plot */
	NULL, /* adaptive_plot */
	NULL, /* vshot */
	NULL, /* tess */
	NULL, /* tnurb */
	NULL, /* brep */
	NULL, /* import5 */
	NULL, /* export5 */
	NULL, /* import4 */
	NULL, /* export4 */
	NULL, /* ifree */
	NULL, /* describe */
	NULL, /* xform */
	NULL, /* parse */
	0, /* sizeof(internal) */
	0, /* magic */
	NULL, /* get */
	NULL, /* adjust */
	NULL, /* form */
	NULL, /* make */
	NULL, /* params */
	NULL, /* bbox */
	NULL, /* volume */
	NULL, /* surf_area */
	NULL, /* centroid */
	NULL, /* oriented_bbox */
	NULL, /* find_selections */
	NULL, /* evaluate_selection */
	NULL, /* process_selection */
	NULL, /* serialize */
	NULL, /* label */
	NULL, /* keypoint */
	NULL, /* mat */
	NULL, /* perturb */
	NULL  /* scene_obj */
    }
};


/* Map for database solidrec objects to internal objects */
static char idmap[] = {
    ID_NULL,	/* undefined, 0 */
    ID_NULL,	/* RPP	1 axis-aligned rectangular parallelepiped */
    ID_NULL,	/* BOX	2 arbitrary rectangular parallelepiped */
    ID_NULL,	/* RAW	3 right-angle wedge */
    ID_NULL,	/* ARB4	4 tetrahedron */
    ID_NULL,	/* ARB5	5 pyramid */
    ID_NULL,	/* ARB6	6 extruded triangle */
    ID_NULL,	/* ARB7	7 weird 7-vertex shape */
    ID_NULL,	/* ARB8	8 hexahedron */
    ID_NULL,	/* ELL	9 ellipsoid */
    ID_NULL,	/* ELL1	10 another ellipsoid ? */
    ID_NULL,	/* SPH	11 sphere */
    ID_NULL,	/* RCC	12 right circular cylinder */
    ID_NULL,	/* REC	13 right elliptic cylinder */
    ID_NULL,	/* TRC	14 truncated regular cone */
    ID_NULL,	/* TEC	15 truncated elliptic cone */
    ID_TOR,	/* TOR	16 toroid */
    ID_NULL,	/* TGC	17 truncated general cone */
    ID_TGC,	/* GENTGC 18 supergeneralized TGC; internal form */
    ID_ELL,	/* GENELL 19: V, A, B, C */
    ID_ARB8,	/* GENARB8 20:  V, and 7 other vectors */
    ID_NULL,	/* HACK: ARS 21: arbitrary triangular-surfaced polyhedron */
    ID_NULL,	/* HACK: ARSCONT 22: extension record type for ARS solid */
    ID_NULL,	/* ELLG 23:  gift-only */
    ID_HALF,	/* HALFSPACE 24:  halfspace */
    ID_NULL,	/* HACK: SPLINE 25 */
    ID_RPC,	/* HACK: RPC 26: right parabolic cylinder */
    ID_RHC,	/* HACK: RHC 27: right hyperbolic cylinder */
    ID_EPA,	/* HACK: EPA 28: elliptical paraboloid */
    ID_EHY,	/* HACK: EHY 29: elliptical hyperboloid */
    ID_ETO,	/* HACK: ETO 29: elliptical torus */
    ID_GRIP,	/* HACK: GRP 30: grip pseudo solid */
    ID_NULL	/* n+1 */
};


/**
 * Given a database record, determine the proper rt_functab subscript.
 * Used by MGED as well as internally to librt.
 *
 * Returns ID_xxx if successful, or ID_NULL upon failure.
 */
int
rt_id_solid(struct bu_external *ep)
{
    union record *rec;
    int id;

    BU_CK_EXTERNAL(ep);
    rec = (union record *)ep->ext_buf;

    switch (rec->u_id) {
	case ID_SOLID:
	    id = idmap[(int)(rec->s.s_type)];
	    break;
	case ID_ARS_A:
	    id = ID_ARS;
	    break;
	case ID_P_HEAD:
	    id = ID_POLY;
	    break;
	case ID_BSOLID:
	    id = ID_BSPLINE;
	    break;
	case DBID_STRSOL:
	    /* XXX This really needs to be some kind of table */
	    if (BU_STR_EQUAL(rec->ss.ss_keyword, "ebm")) {
		id = ID_EBM;
		break;
	    } else if (BU_STR_EQUAL(rec->ss.ss_keyword, "vol")) {
		id = ID_VOL;
		break;
	    } else if (BU_STR_EQUAL(rec->ss.ss_keyword, "hf")) {
		id = ID_HF;
		break;
	    } else if (BU_STR_EQUAL(rec->ss.ss_keyword, "dsp")) {
		id = ID_DSP;
		break;
	    } else if (BU_STR_EQUAL(rec->ss.ss_keyword, "submodel")) {
		id = ID_SUBMODEL;
		break;
	    }
	    bu_log("rt_id_solid(%s):  String solid type '%s' unknown\n",
		    rec->ss.ss_name, rec->ss.ss_keyword);
	    id = ID_NULL;		/* BAD */
	    break;
	case DBID_ARBN:
	    id = ID_ARBN;
	    break;
	case DBID_PIPE:
	    id = ID_PIPE;
	    break;
	case DBID_PARTICLE:
	    id = ID_PARTICLE;
	    break;
	case DBID_NMG:
	    id = ID_NMG;
	    break;
	case DBID_SKETCH:
	    id = ID_SKETCH;
	    break;
	case DBID_ANNOT:
	    id = ID_ANNOT;
	    break;
	case DBID_EXTR:
	    id = ID_EXTRUDE;
	    break;
	case DBID_CLINE:
	    id = ID_CLINE;
	    break;
	case DBID_BOT:
	    id = ID_BOT;
	    break;
	case DBID_SCRIPT:
	    id = ID_SCRIPT;
	    break; 
	default:
	    bu_log("rt_id_solid:  u_id=x%x unknown\n", rec->u_id);
	    id = ID_NULL;		/* BAD */
	    break;
    }
    if (id < ID_NULL || id > ID_MAX_SOLID) {
	bu_log("rt_id_solid: internal error, id=%d?\n", id);
	id = ID_NULL;		/* very BAD */
    }
    return id;
}


/**
 * Given the Tcl 'label' for a given solid type, find the appropriate
 * entry in OBJ[].
 */
const struct rt_functab *
rt_get_functab_by_label(const char *label)
{
    const struct rt_functab *ftp;

    for (ftp = OBJ; ftp->magic != 0; ftp++) {
	if (bu_strncmp(label, ftp->ft_label, 8) == 0)
	    return ftp;
    }
    return NULL;
}


} /* end extern "C" */

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

