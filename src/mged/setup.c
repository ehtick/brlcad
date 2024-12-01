/*                         S E T U P . C
 * BRL-CAD
 *
 * Copyright (c) 1985-2024 United States Government as represented by
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
/** @file mged/setup.c
 *
 * routines to initialize mged
 *
 */

#include "common.h"

/* system headers */
#include <stdlib.h>
#include <tcl.h>
#include <string.h>

/* common headers */
#include "vmath.h"
#include "bu/app.h"
#include "bn.h"
#include "bv/util.h"
#include "tclcad.h"
#include "ged.h"

/* local headers */
#include "./mged.h"
#include "./cmd.h"

/* catch auto-formatting errors in this file.  be careful as there are
 * mged_display() vars that use comma too.
 */
#define COMMA ','


extern Tk_Window tkwin; /* in cmd.c */

extern void init_qray(void);
extern void mged_global_variable_setup(Tcl_Interp *interpreter);

const char cmd3525[] = {'3', '5', COMMA, '2', '5', '\0'};
const char cmd4545[] = {'4', '5', COMMA, '4', '5', '\0'};

// We need to trigger MGED operations when opening and closing
// database files.  However, some commands like garbage_collect
// also need to do these operations, and they have no awareness
// of the extra steps MGED takes with f_opendb/f_closedb.  To
// allow both MGED and GED to do what they need, we define
// default callbacks in GEDP with MGED functions and data that
// will do the necessary work if the opendb/closedb functions
// are called at lower levels.
struct mged_opendb_ctx mged_global_db_ctx;

static struct cmdtab mged_cmdtab[] = {
    {"%", f_comm, GED_FUNC_PTR_NULL},
    {cmd3525, f_bv_35_25, GED_FUNC_PTR_NULL}, /* 35,25 */
    {"3ptarb", cmd_ged_more_wrapper, ged_exec_3ptarb},
    {cmd4545, f_bv_45_45, GED_FUNC_PTR_NULL}, /* 45,45 */
    {"B", cmd_blast, GED_FUNC_PTR_NULL},
    {"accept", f_be_accept, GED_FUNC_PTR_NULL},
    {"adc", f_adc, GED_FUNC_PTR_NULL},
    {"adjust", cmd_ged_plain_wrapper, ged_exec_adjust},
    {"ae", cmd_ged_view_wrapper, ged_exec_ae},
    {"ae2dir", cmd_ged_plain_wrapper, ged_exec_ae2dir},
    {"aip", f_aip, GED_FUNC_PTR_NULL},
    {"analyze", cmd_ged_info_wrapper, ged_exec_analyze},
    {"annotate", cmd_ged_plain_wrapper, ged_exec_annotate},
    {"arb", cmd_ged_plain_wrapper, ged_exec_arb},
    {"arced", cmd_ged_plain_wrapper, ged_exec_arced},
    {"area", f_area, GED_FUNC_PTR_NULL},
    {"arot", cmd_arot, GED_FUNC_PTR_NULL},
    {"art", cmd_rt, GED_FUNC_PTR_NULL},
    {"attach", f_attach, GED_FUNC_PTR_NULL},
    {"attr", cmd_ged_plain_wrapper, ged_exec_attr},
    {"autoview", cmd_autoview, GED_FUNC_PTR_NULL},
    {"bb", cmd_ged_plain_wrapper, ged_exec_bb},
    {"bev", cmd_ged_plain_wrapper, ged_exec_bev},
    {"bo", cmd_ged_plain_wrapper, ged_exec_bo},
    {"bomb", f_bomb, GED_FUNC_PTR_NULL},
    {"bot", cmd_ged_plain_wrapper, ged_exec_bot},
    {"bot_condense", cmd_ged_plain_wrapper, ged_exec_bot_condense},
    {"bot_decimate", cmd_ged_plain_wrapper, ged_exec_bot_decimate},
    {"bot_dump", cmd_ged_plain_wrapper, ged_exec_bot_dump},
    {"bot_exterior", cmd_ged_plain_wrapper, ged_exec_bot_exterior},
    {"bot_face_fuse", cmd_ged_plain_wrapper, ged_exec_bot_face_fuse},
    {"bot_face_sort", cmd_ged_plain_wrapper, ged_exec_bot_face_sort},
    {"bot_flip", cmd_ged_plain_wrapper, ged_exec_bot_flip},
    {"bot_fuse", cmd_ged_plain_wrapper, ged_exec_bot_fuse},
    {"bot_merge", cmd_ged_plain_wrapper, ged_exec_bot_merge},
    {"bot_smooth", cmd_ged_plain_wrapper, ged_exec_bot_smooth},
    {"bot_split", cmd_ged_plain_wrapper, ged_exec_bot_split},
    {"bot_sync", cmd_ged_plain_wrapper, ged_exec_bot_sync},
    {"bot_vertex_fuse", cmd_ged_plain_wrapper, ged_exec_bot_vertex_fuse},
    {"bottom", f_bv_bottom, GED_FUNC_PTR_NULL},
    {"brep", cmd_ged_view_wrapper, ged_exec_brep},
    {"c", cmd_ged_plain_wrapper, ged_exec_c},
    {"cat", cmd_ged_info_wrapper, ged_exec_cat},
    {"cc", cmd_ged_plain_wrapper, ged_exec_cc},
    {"center", cmd_center, GED_FUNC_PTR_NULL},
    {"check", cmd_ged_plain_wrapper, ged_exec_check},
    {"clone", cmd_ged_edit_wrapper, ged_exec_clone},
    {"closedb", f_closedb, GED_FUNC_PTR_NULL},
    {"cmd_win", cmd_cmd_win, GED_FUNC_PTR_NULL},
    {"coil", cmd_ged_plain_wrapper, ged_exec_coil},
    {"color", cmd_ged_plain_wrapper, ged_exec_color},
    {"comb", cmd_ged_plain_wrapper, ged_exec_comb},
    {"comb_color", cmd_ged_plain_wrapper, ged_exec_comb_color},
    {"constraint", cmd_ged_plain_wrapper, ged_exec_constraint},
    {"copyeval", cmd_ged_plain_wrapper, ged_exec_copyeval},
    {"copymat", cmd_ged_plain_wrapper, ged_exec_copymat},
    {"cp", cmd_ged_plain_wrapper, ged_exec_cp},
    {"cpi", f_copy_inv, GED_FUNC_PTR_NULL},
    {"d", cmd_ged_erase_wrapper, ged_exec_d},
    {"db", cmd_stub, GED_FUNC_PTR_NULL},
    {"db_glob", cmd_ged_plain_wrapper, ged_exec_db_glob},
    {"dbconcat", cmd_ged_plain_wrapper, ged_exec_dbconcat},
    {"dbfind", cmd_ged_info_wrapper, ged_exec_dbfind},
    {"dbip", cmd_ged_plain_wrapper, ged_exec_dbip},  // TODO - this needs to go away
    {"dbversion", cmd_ged_plain_wrapper, ged_exec_dbversion},
    {"debug", cmd_ged_plain_wrapper, ged_exec_debug},
    {"debugbu", cmd_ged_plain_wrapper, ged_exec_debugbu},
    {"debugdir", cmd_ged_plain_wrapper, ged_exec_debugdir},
    {"debuglib", cmd_ged_plain_wrapper, ged_exec_debuglib},
    {"debugnmg", cmd_ged_plain_wrapper, ged_exec_debugnmg},
    {"decompose", cmd_ged_plain_wrapper, ged_exec_decompose},
    {"delay", cmd_ged_plain_wrapper, ged_exec_delay},
    {"dir2ae", cmd_ged_plain_wrapper, ged_exec_dir2ae},
    {"dump", cmd_ged_plain_wrapper, ged_exec_dump},
    {"dm", f_dm, GED_FUNC_PTR_NULL},
    {"draw", cmd_draw, GED_FUNC_PTR_NULL},
    {"dsp", cmd_ged_plain_wrapper, ged_exec_dsp},
    {"dup", cmd_ged_plain_wrapper, ged_exec_dup},
    {"E", cmd_E, GED_FUNC_PTR_NULL},
    {"e", cmd_draw, GED_FUNC_PTR_NULL},
    {"eac", cmd_ged_view_wrapper, ged_exec_eac},
    {"echo", cmd_ged_plain_wrapper, ged_exec_echo},
    {"edcodes", f_edcodes, GED_FUNC_PTR_NULL},
    {"edit", cmd_ged_plain_wrapper, ged_exec_edit},
    {"color", cmd_ged_plain_wrapper, ged_exec_color},
    {"edcolor", f_edcolor, GED_FUNC_PTR_NULL},
    {"edcomb", cmd_ged_plain_wrapper, ged_exec_edcomb},
    {"edgedir", f_edgedir, GED_FUNC_PTR_NULL},
    {"edmater", f_edmater, GED_FUNC_PTR_NULL},
    {"env", cmd_ged_plain_wrapper, ged_exec_env},
    {"erase", cmd_ged_erase_wrapper, ged_exec_erase},
    {"ev", cmd_ev, GED_FUNC_PTR_NULL},
    {"eqn", f_eqn, GED_FUNC_PTR_NULL},
    {"exit", f_quit, GED_FUNC_PTR_NULL},
    {"expand", cmd_ged_plain_wrapper, ged_exec_expand},
    {"extrude", f_extrude, GED_FUNC_PTR_NULL},
    {"eye_pt", cmd_ged_view_wrapper, ged_exec_eye_pt},
    {"exists", cmd_ged_plain_wrapper, ged_exec_exists},
    {"facedef", f_facedef, GED_FUNC_PTR_NULL},
    {"facetize", cmd_ged_plain_wrapper, ged_exec_facetize},
    {"facetize_old", cmd_ged_plain_wrapper, ged_exec_facetize_old},
    {"form", cmd_ged_plain_wrapper, ged_exec_form},
    {"fracture", cmd_ged_plain_wrapper, ged_exec_fracture},
    {"front", f_bv_front, GED_FUNC_PTR_NULL},
    {"g", cmd_ged_plain_wrapper, ged_exec_g},
    {"gdiff", cmd_ged_plain_wrapper, ged_exec_gdiff},
    {"garbage_collect", cmd_ged_plain_wrapper, ged_exec_garbage_collect},
    {"get", cmd_ged_plain_wrapper, ged_exec_get},
    {"get_type", cmd_ged_plain_wrapper, ged_exec_get_type},
    {"get_autoview", cmd_ged_plain_wrapper, ged_exec_get_autoview},
    {"get_comb", cmd_ged_plain_wrapper, ged_exec_get_comb},
    {"get_dbip", cmd_ged_plain_wrapper, ged_exec_get_dbip}, // TODO - this needs to go away
    {"get_dm_list", f_get_dm_list, GED_FUNC_PTR_NULL},
    {"get_more_default", cmd_get_more_default, GED_FUNC_PTR_NULL},
    {"get_sed", f_get_sedit, GED_FUNC_PTR_NULL},
    {"get_sed_menus", f_get_sedit_menus, GED_FUNC_PTR_NULL},
    {"get_solid_keypoint", f_get_solid_keypoint, GED_FUNC_PTR_NULL},
    {"graph", cmd_ged_plain_wrapper, ged_exec_graph},
    {"gqa", cmd_ged_gqa, ged_exec_gqa},
    {"grid2model_lu", cmd_ged_plain_wrapper, ged_exec_grid2model_lu},
    {"grid2view_lu", cmd_ged_plain_wrapper, ged_exec_grid2view_lu},
    {"has_embedded_fb", cmd_has_embedded_fb, GED_FUNC_PTR_NULL},
    {"heal", cmd_ged_plain_wrapper, ged_exec_heal},
    {"hide", cmd_ged_plain_wrapper, ged_exec_hide},
    {"hist", cmd_hist, GED_FUNC_PTR_NULL},
    {"history", f_history, GED_FUNC_PTR_NULL},
    {"i", cmd_ged_plain_wrapper, ged_exec_i},
    {"idents", cmd_ged_plain_wrapper, ged_exec_idents},
    {"ill", f_ill, GED_FUNC_PTR_NULL},
    {"in", cmd_ged_in, ged_exec_in},
    {"inside", cmd_ged_inside, ged_exec_inside},
    {"item", cmd_ged_plain_wrapper, ged_exec_item},
    {"joint", cmd_ged_plain_wrapper, ged_exec_joint},
    {"joint2", cmd_ged_plain_wrapper, ged_exec_joint2},
    {"journal", f_journal, GED_FUNC_PTR_NULL},
    {"keep", cmd_ged_plain_wrapper, ged_exec_keep},
    {"keypoint", f_keypoint, GED_FUNC_PTR_NULL},
    {"kill", cmd_ged_erase_wrapper, ged_exec_kill},
    {"killall", cmd_ged_erase_wrapper, ged_exec_killall},
    {"killrefs", cmd_ged_erase_wrapper, ged_exec_killrefs},
    {"killtree", cmd_ged_erase_wrapper, ged_exec_killtree},
    {"knob", f_knob, GED_FUNC_PTR_NULL},
    {"l", cmd_ged_info_wrapper, ged_exec_l},
    {"labelvert", cmd_ged_view_wrapper, ged_exec_labelvert},
    {"labelface", cmd_ged_view_wrapper, ged_exec_labelface},
    {"lc", cmd_ged_plain_wrapper, ged_exec_lc},
    {"left", f_bv_left, GED_FUNC_PTR_NULL},
    {"lint", cmd_ged_plain_wrapper, ged_exec_lint},
    {"listeval", cmd_ged_plain_wrapper, ged_exec_listeval},
    {"loadtk", cmd_tk, GED_FUNC_PTR_NULL},
    {"loadview", cmd_ged_view_wrapper, ged_exec_loadview},
    {"lod", cmd_ged_plain_wrapper, ged_exec_lod},
    {"lookat", cmd_ged_view_wrapper, ged_exec_lookat},
    {"ls", cmd_ged_plain_wrapper, ged_exec_ls},
    {"lt", cmd_ged_plain_wrapper, ged_exec_lt},
    {"M", f_mouse, GED_FUNC_PTR_NULL},
    {"m2v_point", cmd_ged_plain_wrapper, ged_exec_m2v_point},
    {"make", f_make, GED_FUNC_PTR_NULL},
    {"make_name", cmd_ged_plain_wrapper, ged_exec_make_name},
    {"make_pnts", cmd_ged_more_wrapper, ged_exec_make_pnts},
    {"match", cmd_ged_plain_wrapper, ged_exec_match},
    {"mater", cmd_ged_plain_wrapper, ged_exec_mater},
    {"material", cmd_ged_plain_wrapper, ged_exec_material},
    {"matpick", f_matpick, GED_FUNC_PTR_NULL},
    {"mat_ae", cmd_ged_plain_wrapper, ged_exec_mat_ae},
    {"mat_mul", cmd_ged_plain_wrapper, ged_exec_mat_mul},
    {"mat4x3pnt", cmd_ged_plain_wrapper, ged_exec_mat4x3pnt},
    {"mat_scale_about_pnt", cmd_ged_plain_wrapper, ged_exec_mat_scale_about_pnt},
    {"mged_update", f_update, GED_FUNC_PTR_NULL},
    {"mged_wait", f_wait, GED_FUNC_PTR_NULL},
    {"mirface", f_mirface, GED_FUNC_PTR_NULL},
    {"mirror", cmd_ged_plain_wrapper, ged_exec_mirror},
    {"mmenu_get", cmd_mmenu_get, GED_FUNC_PTR_NULL},
    {"mmenu_set", cmd_nop, GED_FUNC_PTR_NULL},
    {"model2grid_lu", cmd_ged_plain_wrapper, ged_exec_model2grid_lu},
    {"model2view", cmd_ged_plain_wrapper, ged_exec_model2view},
    {"model2view_lu", cmd_ged_plain_wrapper, ged_exec_model2view_lu},
    {"mrot", cmd_mrot, GED_FUNC_PTR_NULL},
    {"mv", cmd_ged_plain_wrapper, ged_exec_mv},
    {"mvall", cmd_ged_plain_wrapper, ged_exec_mvall},
    {"nirt", f_nirt, GED_FUNC_PTR_NULL},
    {"nmg_collapse", cmd_nmg_collapse, GED_FUNC_PTR_NULL},
    {"nmg_fix_normals", cmd_ged_plain_wrapper, ged_exec_nmg_fix_normals},
    {"nmg_simplify", cmd_ged_plain_wrapper, ged_exec_nmg_simplify},
    {"nmg", cmd_ged_plain_wrapper, ged_exec_nmg},
    {"npush", cmd_ged_plain_wrapper, ged_exec_npush},
    {"o_rotate", f_be_o_rotate, GED_FUNC_PTR_NULL},
    {"o_scale", f_be_o_scale, GED_FUNC_PTR_NULL},
    {"oed", cmd_oed, GED_FUNC_PTR_NULL},
    {"oed_apply", f_oedit_apply, GED_FUNC_PTR_NULL},
    {"oed_reset", f_oedit_reset, GED_FUNC_PTR_NULL},
    {"oill", f_be_o_illuminate, GED_FUNC_PTR_NULL},
    {"opendb", f_opendb, GED_FUNC_PTR_NULL},
    {"orientation", cmd_ged_view_wrapper, ged_exec_orientation},
    {"orot", f_rot_obj, GED_FUNC_PTR_NULL},
    {"oscale", f_sc_obj, GED_FUNC_PTR_NULL},
    {"output_hook", cmd_output_hook, GED_FUNC_PTR_NULL},
    {"overlay", cmd_overlay, GED_FUNC_PTR_NULL},
    {"ox", f_be_o_x, GED_FUNC_PTR_NULL},
    {"oxscale", f_be_o_xscale, GED_FUNC_PTR_NULL},
    {"oxy", f_be_o_xy, GED_FUNC_PTR_NULL},
    {"oy", f_be_o_y, GED_FUNC_PTR_NULL},
    {"oyscale", f_be_o_yscale, GED_FUNC_PTR_NULL},
    {"ozscale", f_be_o_zscale, GED_FUNC_PTR_NULL},
    {"p", f_param, GED_FUNC_PTR_NULL},
    {"pathlist", cmd_ged_plain_wrapper, ged_exec_pathlist},
    {"paths", cmd_ged_plain_wrapper, ged_exec_paths},
    {"permute", f_permute, GED_FUNC_PTR_NULL},
    {"plot", cmd_ged_plain_wrapper, ged_exec_plot},
    {"png", cmd_ged_plain_wrapper, ged_exec_png},
    {"pnts", cmd_ged_plain_wrapper, ged_exec_pnts},
    {"prcolor", cmd_ged_plain_wrapper, ged_exec_prcolor},
    {"prefix", cmd_ged_plain_wrapper, ged_exec_prefix},
    {"press", f_press, GED_FUNC_PTR_NULL},
    {"preview", cmd_ged_dm_wrapper, ged_exec_preview},
    {"process", cmd_ged_plain_wrapper, ged_exec_process},
    {"postscript", f_postscript, GED_FUNC_PTR_NULL},
    {"ps", cmd_ps, GED_FUNC_PTR_NULL},
    {"pull", cmd_ged_plain_wrapper, ged_exec_pull},
    {"push", cmd_ged_plain_wrapper, ged_exec_push},
    {"put", cmd_ged_plain_wrapper, ged_exec_put},
    {"put_comb", cmd_ged_plain_wrapper, ged_exec_put_comb},
    {"put_sed", f_put_sedit, GED_FUNC_PTR_NULL},
    {"putmat", cmd_ged_plain_wrapper, ged_exec_putmat},
    {"q", f_quit, GED_FUNC_PTR_NULL},
    {"qorot", f_qorot, GED_FUNC_PTR_NULL},
    {"qray", cmd_ged_plain_wrapper, ged_exec_qray},
    {"query_ray", f_nirt, GED_FUNC_PTR_NULL},
    {"quit", f_quit, GED_FUNC_PTR_NULL},
    {"qvrot", cmd_ged_view_wrapper, ged_exec_qvrot},
    {"r", cmd_ged_plain_wrapper, ged_exec_r},
    {"rate", f_bv_rate_toggle, GED_FUNC_PTR_NULL},
    {"rcodes", cmd_ged_plain_wrapper, ged_exec_rcodes},
    {"rear", f_bv_rear, GED_FUNC_PTR_NULL},
    {"red", f_red, GED_FUNC_PTR_NULL},
    {"refresh", f_refresh, GED_FUNC_PTR_NULL},
    {"regdebug", f_regdebug, GED_FUNC_PTR_NULL},
    {"regdef", cmd_ged_plain_wrapper, ged_exec_regdef},
    {"regions", cmd_ged_plain_wrapper, ged_exec_regions},
    {"reject", f_be_reject, GED_FUNC_PTR_NULL},
    {"release", f_release, GED_FUNC_PTR_NULL},
    {"reset", f_bv_reset, GED_FUNC_PTR_NULL},
    {"restore", f_bv_vrestore, GED_FUNC_PTR_NULL},
    {"rfarb", f_rfarb, GED_FUNC_PTR_NULL},
    {"right", f_bv_right, GED_FUNC_PTR_NULL},
    {"rm", cmd_ged_plain_wrapper, ged_exec_rm},
    {"rmater", cmd_ged_plain_wrapper, ged_exec_rmater},
    {"rmats", f_rmats, GED_FUNC_PTR_NULL},
    {"rot", cmd_rot, GED_FUNC_PTR_NULL},
    {"rotobj", f_rot_obj, GED_FUNC_PTR_NULL},
    {"rrt", cmd_rrt, GED_FUNC_PTR_NULL},
    {"rset", f_rset, GED_FUNC_PTR_NULL},
    {"rt", cmd_rt, GED_FUNC_PTR_NULL},
    {"rt_gettrees", cmd_rt_gettrees, GED_FUNC_PTR_NULL},
    {"rtabort", cmd_ged_plain_wrapper, ged_exec_rtabort},
    {"rtarea", cmd_rt, GED_FUNC_PTR_NULL},
    {"rtcheck", cmd_rt, GED_FUNC_PTR_NULL},
    {"rtedge", cmd_rt, GED_FUNC_PTR_NULL},
    {"rtweight", cmd_rt, GED_FUNC_PTR_NULL},
    {"save", f_bv_vsave, GED_FUNC_PTR_NULL},
    {"savekey", cmd_ged_plain_wrapper, ged_exec_savekey},
    {"saveview", cmd_ged_plain_wrapper, ged_exec_saveview},
    {"sca", cmd_sca, GED_FUNC_PTR_NULL},
    {"screengrab", cmd_ged_dm_wrapper, ged_exec_screengrab},
    {"search", cmd_search, GED_FUNC_PTR_NULL},
    {"sed", f_sed, GED_FUNC_PTR_NULL},
    {"sed_apply", f_sedit_apply, GED_FUNC_PTR_NULL},
    {"sed_reset", f_sedit_reset, GED_FUNC_PTR_NULL},
    {"sedit", f_be_s_edit, GED_FUNC_PTR_NULL},
    {"select", cmd_ged_plain_wrapper, ged_exec_select},
    {"set_more_default", cmd_set_more_default, GED_FUNC_PTR_NULL},
    {"setview", cmd_setview, GED_FUNC_PTR_NULL},
    {"shaded_mode", cmd_shaded_mode, GED_FUNC_PTR_NULL},
    {"shader", cmd_ged_plain_wrapper, ged_exec_shader},
    {"share", f_share, GED_FUNC_PTR_NULL},
    {"shells", cmd_ged_plain_wrapper, ged_exec_shells},
    {"showmats", cmd_ged_plain_wrapper, ged_exec_showmats},
    {"sill", f_be_s_illuminate, GED_FUNC_PTR_NULL},
    {"size", cmd_size, GED_FUNC_PTR_NULL},
    {"simulate", cmd_ged_simulate_wrapper, ged_exec_simulate},
    {"solid_report", cmd_ged_plain_wrapper, ged_exec_solid_report},
    {"solids", cmd_ged_plain_wrapper, ged_exec_solids},
    {"solids_on_ray", cmd_ged_plain_wrapper, ged_exec_solids_on_ray},
    {"srot", f_be_s_rotate, GED_FUNC_PTR_NULL},
    {"sscale", f_be_s_scale, GED_FUNC_PTR_NULL},
    {"stat", cmd_ged_plain_wrapper, ged_exec_stat},
    {"status", f_status, GED_FUNC_PTR_NULL},
    {"stuff_str", cmd_stuff_str, GED_FUNC_PTR_NULL},
    {"summary", cmd_ged_plain_wrapper, ged_exec_summary},
    {"sv", f_slewview, GED_FUNC_PTR_NULL},
    {"svb", f_svbase, GED_FUNC_PTR_NULL},
    {"sxy", f_be_s_trans, GED_FUNC_PTR_NULL},
    {"sync", cmd_ged_plain_wrapper, ged_exec_sync},
    {"t", cmd_ged_plain_wrapper, ged_exec_t},
    {"ted", f_tedit, GED_FUNC_PTR_NULL},
    {"tie", f_tie, GED_FUNC_PTR_NULL},
    {"tire", cmd_ged_plain_wrapper, ged_exec_tire},
    {"title", cmd_ged_plain_wrapper, ged_exec_title},
    {"tol", cmd_tol, GED_FUNC_PTR_NULL},
    {"top", f_bv_top, GED_FUNC_PTR_NULL},
    {"tops", cmd_ged_plain_wrapper, ged_exec_tops},
    {"tra", cmd_tra, GED_FUNC_PTR_NULL},
    {"track", f_amtrack, GED_FUNC_PTR_NULL},
    {"tracker", f_tracker, GED_FUNC_PTR_NULL},
    {"translate", f_tr_obj, GED_FUNC_PTR_NULL},
    {"tree", cmd_ged_plain_wrapper, ged_exec_tree},
    {"unhide", cmd_ged_plain_wrapper, ged_exec_unhide},
    {"units", cmd_units, GED_FUNC_PTR_NULL},
    {"v2m_point", cmd_ged_plain_wrapper, ged_exec_v2m_point},
    {"vars", f_set, GED_FUNC_PTR_NULL},
    {"vdraw", cmd_ged_plain_wrapper, ged_exec_vdraw},
    {"view", cmd_ged_view_wrapper, ged_exec_view},
    {"view_ring", f_view_ring, GED_FUNC_PTR_NULL},
    {"view2grid_lu", cmd_ged_plain_wrapper, ged_exec_view2grid_lu},
    {"view2model", cmd_ged_plain_wrapper, ged_exec_view2model},
    {"view2model_lu", cmd_ged_plain_wrapper, ged_exec_view2model_lu},
    {"view2model_vec", cmd_ged_plain_wrapper, ged_exec_view2model_vec},
    {"viewdir", cmd_ged_plain_wrapper, ged_exec_viewdir},
    {"viewsize", cmd_size, GED_FUNC_PTR_NULL}, /* alias "size" for saveview scripts */
    {"vnirt", f_vnirt, GED_FUNC_PTR_NULL},
    {"voxelize", cmd_ged_plain_wrapper, ged_exec_voxelize},
    {"vquery_ray", f_vnirt, GED_FUNC_PTR_NULL},
    {"vrot", cmd_vrot, GED_FUNC_PTR_NULL},
    {"wcodes", cmd_ged_plain_wrapper, ged_exec_wcodes},
    {"whatid", cmd_ged_plain_wrapper, ged_exec_whatid},
    {"which_shader", cmd_ged_plain_wrapper, ged_exec_which_shader},
    {"whichair", cmd_ged_plain_wrapper, ged_exec_whichair},
    {"whichid", cmd_ged_plain_wrapper, ged_exec_whichid},
    {"who", cmd_ged_plain_wrapper, ged_exec_who},
    {"winset", f_winset, GED_FUNC_PTR_NULL},
    {"wmater", cmd_ged_plain_wrapper, ged_exec_wmater},
    {"x", cmd_ged_plain_wrapper, ged_exec_x},
    {"xpush", cmd_ged_plain_wrapper, ged_exec_xpush},
    {"Z", cmd_zap, GED_FUNC_PTR_NULL},
    {"zoom", cmd_zoom, GED_FUNC_PTR_NULL},
    {"zoomin", f_bv_zoomin, GED_FUNC_PTR_NULL},
    {"zoomout", f_bv_zoomout, GED_FUNC_PTR_NULL},
    {NULL, NULL, GED_FUNC_PTR_NULL}
};


/**
 * Register all MGED commands.
 */
static void
cmd_setup(void)
{
    struct cmdtab *ctp;
    struct bu_vls temp = BU_VLS_INIT_ZERO;

    // TODO - should be using libged cmd list to populate everything in this
    // table that is a plain wrapper - that way all ged commands are always
    // automatically there and we can delete the hardcoded logic.  Drawback
    // would be we would lose the compile time check that libged has what MGED
    // is expecting... a compromise might be to have mged validate such a list
    // at startup and exit if any expected commands aren't valid.
    //
    // In the case of a conflicting cmd name between the non-plain wrappers and
    // the above table contents the MGED tbl should win, but we should also
    // look into the reasons for the wrappers NOT being plain and try to find a
    // way (callbacks, etc.) to avoid the need for non-plain wrappers.

    for (ctp = mged_cmdtab; ctp->name != NULL; ctp++) {
	bu_vls_strcpy(&temp, "_mged_");
	bu_vls_strcat(&temp, ctp->name);

	(void)Tcl_CreateCommand(INTERP, ctp->name, ctp->tcl_func,
				(ClientData)ctp, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(INTERP, bu_vls_addr(&temp), ctp->tcl_func,
				(ClientData)ctp, (Tcl_CmdDeleteProc *)NULL);
    }

    /* Init mged's Tcl interface to libwdb */
    Wdb_Init(INTERP);

    tkwin = NULL;

    bu_vls_free(&temp);
}


/*
 * Initialize mged, configure the path, set up the tcl interpreter.
 */
void
mged_setup(Tcl_Interp **interpreter)
{
    struct bu_vls str = BU_VLS_INIT_ZERO;
    struct bu_vls tlog = BU_VLS_INIT_ZERO;
    const char *name = bu_dir(NULL, 0, BU_DIR_BIN, bu_getprogname(), BU_DIR_EXT, NULL);

    /* locate our run-time binary (must be called before Tcl_CreateInterp()) */
    if (name) {
	Tcl_FindExecutable(name);
    } else {
	Tcl_FindExecutable("mged");
    }

    if (!interpreter ) {
      bu_log("mged_setup Error - interpreter is NULL!\n");
      return;
    }

    if (*interpreter != NULL)
	Tcl_DeleteInterp(*interpreter);

    /* Create the interpreter */
    *interpreter = Tcl_CreateInterp();

    /* Do basic Tcl initialization - note that Tk
     * is not initialized at this point. */
    if (tclcad_init(*interpreter, 0, &tlog) == TCL_ERROR) {
	bu_log("tclcad_init error:\n%s\n", bu_vls_addr(&tlog));
    }
    bu_vls_free(&tlog);

    GEDP = ged_create();
    GEDP->ged_output_handler = mged_output_handler;
    GEDP->ged_refresh_handler = mged_refresh_handler;
    GEDP->ged_create_vlist_scene_obj_callback = createDListSolid;
    GEDP->ged_create_vlist_display_list_callback = createDListAll;
    GEDP->ged_destroy_vlist_callback = freeDListsAll;
    GEDP->ged_create_io_handler = &tclcad_create_io_handler;
    GEDP->ged_delete_io_handler = &tclcad_delete_io_handler;

    ged_clbk_set(GEDP, "opendb", GED_CLBK_PRE, &mged_pre_opendb_clbk, (void *)&mged_global_db_ctx);
    ged_clbk_set(GEDP, "opendb", GED_CLBK_POST, &mged_post_opendb_clbk, (void *)&mged_global_db_ctx);
    ged_clbk_set(GEDP, "closedb", GED_CLBK_PRE, &mged_pre_closedb_clbk, (void *)&mged_global_db_ctx);
    ged_clbk_set(GEDP, "closedb", GED_CLBK_POST, &mged_post_closedb_clbk, (void *)&mged_global_db_ctx);

    // Register during-execution callback function for search command
    ged_clbk_set(GEDP, "search", GED_CLBK_DURING, &mged_db_search_callback, (void *)*interpreter);

    struct tclcad_io_data *t_iod = tclcad_create_io_data();
    t_iod->io_mode = TCL_READABLE;
    t_iod->interp = *interpreter;
    GEDP->ged_io_data = t_iod;

    /* Set up the default state of the standard open/close db container */
    mged_global_db_ctx.argc = 0;
    mged_global_db_ctx.argv = NULL;
    mged_global_db_ctx.force_create = 0;
    mged_global_db_ctx.no_create = 0;
    mged_global_db_ctx.created_new_db = 0;
    mged_global_db_ctx.ret = 0;
    mged_global_db_ctx.ged_ret = 0;
    mged_global_db_ctx.interpreter = *interpreter;
    mged_global_db_ctx.old_dbip = NULL;
    mged_global_db_ctx.post_open_cnt = 0;

    BU_ALLOC(view_state->vs_gvp, struct bview);
    bv_init(view_state->vs_gvp, NULL);
    BU_GET(view_state->vs_gvp->callbacks, struct bu_ptbl);
    bu_ptbl_init(view_state->vs_gvp->callbacks, 8, "bv callbacks");

    view_state->vs_gvp->gv_callback = mged_view_callback;
    view_state->vs_gvp->gv_clientData = (void *)view_state;
    MAT_DELTAS_GET_NEG(view_state->vs_orig_pos, view_state->vs_gvp->gv_center);

    view_state->vs_gvp->vset = &GEDP->ged_views;

    bv_set_add_view(&GEDP->ged_views, view_state->vs_gvp);
    bu_ptbl_ins(&GEDP->ged_free_views, (long *)view_state->vs_gvp);
    GEDP->ged_gvp = view_state->vs_gvp;

    /* register commands */
    cmd_setup();

    history_setup();
    mged_global_variable_setup(*interpreter);
    mged_variable_setup(*interpreter);
    GEDP->ged_interp = (void *)*interpreter;

    /* Tcl needs to write nulls onto subscripted variable names */
    bu_vls_printf(&str, "%s(state)", MGED_DISPLAY_VAR);
    Tcl_SetVar(*interpreter, bu_vls_addr(&str), state_str[STATE], TCL_GLOBAL_ONLY);

    /* Set defaults for view status variables */
    bu_vls_trunc(&str, 0);
    bu_vls_printf(&str, "set mged_display(.topid_0.ur,ang) {ang=(0.00 0.00 0.00)};\
set mged_display(.topid_0.ur,aet) {az=35.00  el=25.00  tw=0.00};\
set mged_display(.topid_0.ur,size) sz=1000.000;\
set mged_display(.topid_0.ur,center) {cent=(0.000 0.000 0.000)};\
set mged_display(units) mm");
    Tcl_Eval(*interpreter, bu_vls_addr(&str));

    Tcl_ResetResult(*interpreter);

    bu_vls_free(&str);
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
