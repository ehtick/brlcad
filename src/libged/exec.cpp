/*                        E X E C . C P P
 * BRL-CAD
 *
 * Copyright (c) 2020-2025 United States Government as represented by
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

#include "common.h"

#include <map>
#include <string>

#include "bu/time.h"
#include "bu/path.h"
#include "bu/vls.h"
#include "ged.h"
#include "./include/plugin.h"

extern "C" void libged_init(void);


extern "C" int
ged_exec(struct ged *gedp, int argc, const char *argv[])
{
    int cret = BRLCAD_OK;

    if (!gedp || !argc || !argv) {
	return BRLCAD_ERROR;
    }

    /* make sure argc and argv agree, no NULL args */
    for (int i = 0; i < argc; i++) {
	if (!argv[i]) {
	    if (i == (argc -1)) {
		// NULL termination, decrement argc and move on
		argc--;
	    } else {
		bu_log("INTERNAL ERROR: ged_exec() argv[%d] is NULL (argc=%d)\n", i, argc);
		return BRLCAD_ERROR;
	    }
	}
    }

    double start = 0.0;
    const char *tstr = getenv("GED_EXEC_TIME");
    if (tstr) {
	start = bu_gettime();
    }

    // TODO - right now this is the map from the libged load - should probably
    // use this to initialize a struct ged copy when ged_init is called, so
    // client codes can add their own commands to their gedp...
    //
    // The ged_cmds map should always reflect the original, vanilla state of
    // libged's command set so we have a clean fallback available if we ever
    // need it to fall back on/recover with.
    std::map<std::string, const struct ged_cmd *> *cmap = (std::map<std::string, const struct ged_cmd *> *)ged_cmds;

    // On OpenBSD, if the executable was launched in a way that requires
    // bu_setprogname to find the BRL-CAD root directory the initial libged
    // initialization would have failed.  If we have no ged_cmds at all this is
    // probably what happened, so call libged_init again here.  By the time we
    // are calling ged_exec bu_setprogname should be set and we should be ready
    // to actually find the commands.
    if (!cmap->size())
	libged_init();

    /* libged is only concerned with the basename in order for command-line
     * applications to pass an argv[0]. */
    struct bu_vls cmdvls = BU_VLS_INIT_ZERO;
    bu_path_component(&cmdvls, argv[0], BU_PATH_BASENAME);
    std::string cmdname = bu_vls_cstr(&cmdvls);
    bu_vls_free(&cmdvls);

    // Validate the command name.  If we don't know what this is, we can't run
    // it successfully and need to bail.
    std::map<std::string, const struct ged_cmd *>::iterator c_it = cmap->find(cmdname);
    if (c_it == cmap->end()) {
	bu_vls_printf(gedp->ged_result_str, "unknown command: %s", cmdname.c_str());
	return (BRLCAD_ERROR | GED_UNKNOWN);
    }
    const struct ged_cmd *cmd = c_it->second;


    // We have a command now - push it onto the stack
    GED_CK_MAGIC(gedp);
    Ged_Internal *gedip = gedp->i->i;
    gedip->exec_stack.push(cmdname);
    gedip->cmd_recursion_depth_cnt[cmdname]++;

    if (gedip->cmd_recursion_depth_cnt[cmdname] > GED_CMD_RECURSION_LIMIT) {
	bu_vls_printf(gedp->ged_result_str, "Recursion limit %d exceeded for command %s - aborted.  ged_exec call stack:\n", GED_CMD_RECURSION_LIMIT, cmdname.c_str());
	std::stack<std::string> lexec_stack = gedip->exec_stack;
	while (!lexec_stack.empty()) {
	    bu_vls_printf(gedp->ged_result_str, "%s\n", lexec_stack.top().c_str());
	    lexec_stack.pop();
	}
	return BRLCAD_ERROR;
    }

    // Check for a pre-exec callback.
    bu_clbk_t f = NULL;
    void *d = NULL;
    if ((ged_clbk_get(&f, &d, gedp, cmdname.c_str(), BU_CLBK_PRE) == BRLCAD_OK) && f) {
	cret = ged_clbk_exec(gedp->ged_result_str, gedp, GED_CMD_RECURSION_LIMIT, f, argc, argv, gedp, d);
	if (cret != BRLCAD_OK)
	    bu_log("error running %s pre-execution callback\n", cmdname.c_str());
    }

    // TODO - if interactive command via cmd->i->interactive, don't execute
    // unless we have the necessary callbacks defined in gedp

    // Preliminaries complete - do the actual command execution call
    cret = (*cmd->i->cmd)(gedp, argc, argv);

    // If we didn't execute successfully, don't execute the post run hook.  (If
    // a specific command wants to anyway, it can do so in its own
    // implementation.)
    if (cret != BRLCAD_OK) {
	if (tstr)
	    bu_log("%s time: %g\n", cmdname.c_str(), (bu_gettime() - start)/1e6);

	gedip->cmd_recursion_depth_cnt[cmdname]--;
	gedip->exec_stack.pop();
	return cret;
    }

    // Command execution complete - check for a post command callback.
    if ((ged_clbk_get(&f, &d, gedp, cmdname.c_str(), BU_CLBK_POST) == BRLCAD_OK) && f) {
	cret = ged_clbk_exec(gedp->ged_result_str, gedp, GED_CMD_RECURSION_LIMIT, f, argc, argv, gedp, d);
	if (cret != BRLCAD_OK)
	    bu_log("error running %s post-execution callback\n", cmdname.c_str());
    }

    if (tstr)
	bu_log("%s time: %g\n", cmdname.c_str(), (bu_gettime() - start)/1e6);

    gedip->cmd_recursion_depth_cnt[cmdname]--;
    gedip->exec_stack.pop();
    return cret;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
