/*                         P U S H . C
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
/** @file libged/push.c
 *
 * The push command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>


#include "bu/cmd.h"
#include "bu/getopt.h"

#include "../ged_private.h"


#define PUSH_MAGIC_ID 0x50495323
#define FOR_ALL_PUSH_SOLIDS(_p, _phead) \
    for (_p=_phead.forw; _p!=&_phead; _p=_p->forw)

/** structure to hold all solids that have been pushed. */
struct push_id {
    uint32_t magic;
    struct push_id *forw, *back;
    struct directory *pi_dir;
    mat_t pi_mat;
};


struct push_data {
    struct ged *gedp;
    struct push_id pi_head;
    int push_error;
};


static void
do_identitize(struct db_i *dbip, struct rt_comb_internal *UNUSED(comb), union tree *comb_leaf, void *user_ptr1, void *UNUSED(user_ptr2), void *UNUSED(user_ptr3), void *UNUSED(user_ptr4));


/**
 * Traverses an objects paths, setting all member matrices == identity
 */
static void
identitize(struct directory *dp,
	   struct db_i *dbip,
	   struct bu_vls *msg)
{
    struct rt_db_internal intern;
    struct rt_comb_internal *comb;

    if (dp->d_flags & RT_DIR_SOLID)
	return;
    if (rt_db_get_internal(&intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
	bu_vls_printf(msg, "Database read error, aborting\n");
	return;
    }
    comb = (struct rt_comb_internal *)intern.idb_ptr;
    if (comb->tree) {
	db_tree_funcleaf(dbip, comb, comb->tree, do_identitize,
			 (void *)msg, (void *)NULL, (void *)NULL, (void *)NULL);
	if (rt_db_put_internal(dp, dbip, &intern, &rt_uniresource) < 0) {
	    bu_vls_printf(msg, "Cannot write modified combination (%s) to database\n", dp->d_namep);
	    return;
	}
    }
}


/**
 * This routine must be prepared to run in parallel.
 *
 * @brief
 * This routine is called once for eas leaf (solid) that is to
 * be pushed.  All it does is build at push_id linked list.  The
 * linked list could be handled by bu_list macros but it is simple
 * enough to do hear with out them.
 */
static union tree *
push_leaf(struct db_tree_state *tsp,
	  const struct db_full_path *pathp,
	  struct rt_db_internal *ip,
	  void *client_data)
{
    union tree *curtree;
    struct directory *dp;
    struct push_id *gpip;
    struct push_data *gpdp = (struct push_data *)client_data;

    BG_CK_TESS_TOL(tsp->ts_ttol);
    BN_CK_TOL(tsp->ts_tol);
    RT_CK_RESOURCE(tsp->ts_resp);

    dp = pathp->fp_names[pathp->fp_len-1];

    if (RT_G_DEBUG&RT_DEBUG_TREEWALK) {
	char *sofar = db_path_to_string(pathp);

	bu_vls_printf(gpdp->gedp->ged_result_str, "push_leaf(%s) path='%s'\n", ip->idb_meth->ft_name, sofar);
	bu_free((void *)sofar, "path string");
    }
/*
 * XXX - This will work but is not the best method.  dp->d_uses tells us
 * if this solid (leaf) has been seen before.  If it hasn't just add
 * it to the list.  If it has, search the list to see if the matrices
 * match and do the "right" thing.
 *
 * (There is a question as to whether dp->d_uses is reset to zero
 * for each tree walk.  If it is not, then d_uses is NOT a safe
 * way to check and this method will always work.)
 */
    bu_semaphore_acquire(RT_SEM_WORKER);
    FOR_ALL_PUSH_SOLIDS(gpip, gpdp->pi_head) {
	if (gpip->pi_dir == dp) {
	    if (!bn_mat_is_equal(gpip->pi_mat,
				 tsp->ts_mat, tsp->ts_tol)) {
		char *sofar = db_path_to_string(pathp);

		bu_vls_printf(gpdp->gedp->ged_result_str, "push_leaf: matrix mismatch between '%s' and prior reference.\n", sofar);
		bu_free((void *)sofar, "path string");
		gpdp->push_error = 1;
	    }

	    bu_semaphore_release(RT_SEM_WORKER);
	    BU_GET(curtree, union tree);
	    RT_TREE_INIT(curtree);
	    curtree->tr_op = OP_NOP;
	    return curtree;
	}
    }
/*
 * This is the first time we have seen this solid.
 */
    BU_ALLOC(gpip, struct push_id);
    gpip->magic = PUSH_MAGIC_ID;
    gpip->pi_dir = dp;
    MAT_COPY(gpip->pi_mat, tsp->ts_mat);
    gpip->back = gpdp->pi_head.back;
    gpdp->pi_head.back = gpip;
    gpip->forw = &gpdp->pi_head;
    gpip->back->forw = gpip;
    bu_semaphore_release(RT_SEM_WORKER);
    BU_GET(curtree, union tree);
    RT_TREE_INIT(curtree);
    curtree->tr_op = OP_NOP;
    return curtree;
}


/**
 * @brief
 * A null routine that does nothing.
 */
static union tree *
push_region_end(struct db_tree_state *UNUSED(tsp), const struct db_full_path *UNUSED(pathp), union tree *curtree, void *UNUSED(client_data))
{
    return curtree;
}


int
ged_push_core(struct ged *gedp, int argc, const char *argv[])
{
    struct push_data *gpdp;
    struct push_id *gpip;
    struct rt_db_internal es_int;
    int i;
    int ncpu;
    int c;
    int old_debug;
    int push_error;
    static const char *usage = "object(s)";

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

    BU_GET(gpdp, struct push_data);
    gpdp->gedp = gedp;
    gpdp->push_error = 0;
    gpdp->pi_head.magic = PUSH_MAGIC_ID;
    gpdp->pi_head.forw = gpdp->pi_head.back = &gpdp->pi_head;
    gpdp->pi_head.pi_dir = (struct directory *) 0;

    old_debug = RT_G_DEBUG;

    /* Initial values for options, must be reset each time */
    ncpu = 1;

    /* Parse options */
    bu_optind = 1;	/* re-init bu_getopt() */
    while ((c=bu_getopt(argc, (char * const *)argv, "P:d")) != -1) {
	switch (c) {
	    case 'P':
		ncpu = atoi(bu_optarg);
		if (ncpu<1) ncpu = 1;
		break;
	    case 'd':
		rt_debug |= RT_DEBUG_TREEWALK;
		break;
	    case '?':
	    default:
		bu_vls_printf(gedp->ged_result_str, "ged_push_core: usage push [-P processors] [-d] root [root2 ...]\n");
		break;
	}
    }

    argc -= bu_optind;
    argv += bu_optind;

    /*
     * build a linked list of solids with the correct
     * matrix to apply to each solid.  This will also
     * check to make sure that a solid is not pushed in two
     * different directions at the same time.
     */
    struct rt_wdb *wdbp = wdb_dbopen(gedp->dbip, RT_WDB_TYPE_DB_DEFAULT);
    i = db_walk_tree(gedp->dbip, argc, (const char **)argv,
		     ncpu,
		     &wdbp->wdb_initial_tree_state,
		     0,				/* take all regions */
		     push_region_end,
		     push_leaf, (void *)gpdp);

    /*
     * If there was any error, then just free up the solid
     * list we just built.
     */
    if (i < 0 || gpdp->push_error) {
	while (gpdp->pi_head.forw != &gpdp->pi_head) {
	    gpip = gpdp->pi_head.forw;
	    gpip->forw->back = gpip->back;
	    gpip->back->forw = gpip->forw;
	    bu_free((void *)gpip, "Push ident");
	}
	rt_debug = old_debug;
	BU_PUT(gpdp, struct push_data);
	bu_vls_printf(gedp->ged_result_str, "ged_push_core:\tdb_walk_tree failed or there was a solid moving\n\tin two or more directions");
	return BRLCAD_ERROR;
    }
/*
 * We've built the push solid list, now all we need to do is apply
 * the matrix we've stored for each solid.
 */
    FOR_ALL_PUSH_SOLIDS(gpip, gpdp->pi_head) {
	if (rt_db_get_internal(&es_int, gpip->pi_dir, gedp->dbip, gpip->pi_mat, &rt_uniresource) < 0) {
	    bu_vls_printf(gedp->ged_result_str, "ged_push_core: Read error fetching '%s'\n", gpip->pi_dir->d_namep);
	    gpdp->push_error = -1;
	    continue;
	}
	RT_CK_DB_INTERNAL(&es_int);

	if (rt_db_put_internal(gpip->pi_dir, gedp->dbip, &es_int, &rt_uniresource) < 0) {
	    bu_vls_printf(gedp->ged_result_str, "ged_push_core(%s): solid export failure\n", gpip->pi_dir->d_namep);
	}
	rt_db_free_internal(&es_int);
    }

    /*
     * Now use the wdb_identitize() tree walker to turn all the
     * matrices in a combination to the identity matrix.
     * It would be nice to use db_tree_walker() but the tree
     * walker does not give us all combinations, just regions.
     * This would work if we just processed all matrices backwards
     * from the leaf (solid) towards the root, but all in all it
     * seems that this is a better method.
     */

    while (argc > 0) {
	struct directory *db;
	db = db_lookup(gedp->dbip, *argv++, 0);
	if (db)
	    identitize(db, gedp->dbip, gedp->ged_result_str);
	--argc;
    }

    /*
     * Free up the solid table we built.
     */
    while (gpdp->pi_head.forw != &gpdp->pi_head) {
	gpip = gpdp->pi_head.forw;
	gpip->forw->back = gpip->back;
	gpip->back->forw = gpip->forw;
	bu_free((void *)gpip, "Push ident");
    }

    rt_debug = old_debug;
    push_error = gpdp->push_error;
    BU_PUT(gpdp, struct push_data);

    return push_error ? BRLCAD_ERROR : BRLCAD_OK;
}


static void
do_identitize(struct db_i *dbip, struct rt_comb_internal *UNUSED(comb), union tree *comb_leaf, void *user_ptr1, void *UNUSED(user_ptr2), void *UNUSED(user_ptr3), void *UNUSED(user_ptr4))
{
    struct directory *dp;
    struct bu_vls *msg = (struct bu_vls *)user_ptr1;

    RT_CK_DBI(dbip);
    RT_CK_TREE(comb_leaf);

    if (!comb_leaf->tr_l.tl_mat) {
	comb_leaf->tr_l.tl_mat = (matp_t)bu_malloc(sizeof(mat_t), "tl_mat");
    }
    MAT_IDN(comb_leaf->tr_l.tl_mat);
    if ((dp = db_lookup(dbip, comb_leaf->tr_l.tl_name, LOOKUP_NOISY)) == RT_DIR_NULL)
	return;

    identitize(dp, dbip, msg);
}


#ifdef GED_PLUGIN
#include "../include/plugin.h"
struct ged_cmd_impl push_cmd_impl = {
    "push",
    ged_push_core,
    GED_CMD_DEFAULT
};

const struct ged_cmd push_cmd = { &push_cmd_impl };
const struct ged_cmd *push_cmds[] = { &push_cmd, NULL };

static const struct ged_plugin pinfo = { GED_API,  push_cmds, 1 };

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
