/*                       A S C - N M G . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2025 United States Government as represented by
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
 *
 */
/** @file asc-nmg.c
 *
 *  Program to convert an ascii description of an NMG into a BRL-CAD
 *  NMG model.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "bio.h"
#include "vmath.h"
#include "bu/app.h"
#include "bu/getopt.h"
#include "nmg.h"
#include "raytrace.h"
#include "rt/geom.h"
#include "wdb.h"

static int ascii_to_brlcad(FILE *fpin, struct rt_wdb *fpout, char *reg_name, char *grp_name, struct bu_list *vlfree);
static void descr_to_nmg(struct shell *s, FILE *fp, fastf_t *Ext);

void
usage(void)
{
	bu_log("Usage: asc-nmg [filein] [fileout] ; use - for stdin\n");
}

/*
 *	Get ascii input file and output file names.
 */
int
main(int argc, char **argv)
{
    char		*afile = NULL;
    char		*bfile = "nmg.g";
    FILE		*fpin;
    struct rt_wdb	*fpout;
    RT_VLFREE_INIT();
    struct bu_list *vlfree = &rt_vlfree;

    if ( BU_STR_EQUAL(argv[1],"-h") || BU_STR_EQUAL(argv[1],"-?")) {
	usage();
	bu_exit(1, NULL);
    }

    if (isatty(fileno(stdin)) && isatty(fileno(stdout)) && argc == 1) {
	usage();
    }

    bu_setprogname(argv[0]);

    /* Get ascii NMG input file name. */
    if (bu_optind >= argc || (int)(*argv[1]) == '-') {
	fpin = stdin;
	setmode(fileno(fpin), O_BINARY);
	bu_log("%s: will be reading from stdin\n",argv[0]);
    } else {
	afile = argv[bu_optind];
	if ((fpin = fopen(afile, "rb")) == NULL) {
	    fprintf(stderr,
		    "%s: cannot open %s for reading\n",
		    argv[0], afile);
	    bu_exit(1, NULL);
	}
	bu_log("%s: will be reading from file %s\n",argv[0],afile);
    }

    /* Get BRL-CAD output data base name. */
    bu_optind++;
    if (bu_optind < argc)
	bfile = argv[bu_optind];
    if ((fpout = wdb_fopen(bfile)) == NULL) {
	fprintf(stderr, "%s: cannot open %s for writing\n",
		argv[0], bfile);
	bu_exit(1, NULL);
    }
    bu_log("%s: will be creating file %s\n",argv[0],bfile);

    ascii_to_brlcad(fpin, fpout, "nmg", NULL, vlfree);
    fclose(fpin);
    db_close(fpout->dbip);
    return 0;
}

/*
 *	Write the nmg to a BRL-CAD style data base.
 */
void
create_brlcad_db(struct rt_wdb *fpout, struct model *m, char *reg_name, char *grp_name)
{
    char	*rname, *sname;
    int size = sizeof(reg_name) + 3;

    mk_id(fpout, "Ascii NMG");

    rname = (char *)bu_malloc(size, "rname");	/* Region name. */
    sname = (char *)bu_malloc(size, "sname");	/* Solid name. */

    snprintf(sname, size, "s.%s", reg_name);
    mk_nmg(fpout, sname,  m);		/* Make nmg object. */
    snprintf(rname, size, "r.%s", reg_name);
    mk_comb1(fpout, rname, sname, 1);	/* Put object in a region. */
    if (grp_name) {
	mk_comb1(fpout, grp_name, rname, 1);	/* Region in group. */
    }
}

/*
 *	Convert an ascii nmg description into a BRL-CAD data base.
 */
static int
ascii_to_brlcad(FILE *fpin, struct rt_wdb *fpout, char *reg_name, char *grp_name, struct bu_list *vlfree)
{
    struct model	*m;
    struct nmgregion	*r;
    struct bn_tol	tol;
    struct shell	*s;
    vect_t		Ext;
    struct faceuse *fu;
    plane_t		pl;

    VSETALL(Ext, 0.);

    m = nmg_mm();		/* Make nmg model. */
    r = nmg_mrsv(m);	/* Make region, empty shell, vertex */
    s = BU_LIST_FIRST(shell, &r->s_hd);
    descr_to_nmg(s, fpin, Ext);	/* Convert ascii description to nmg. */

    /* Copied from proc-db/nmgmodel.c */
    tol.magic = BN_TOL_MAGIC;
    tol.dist = 0.01;
    tol.dist_sq = tol.dist * tol.dist;
    tol.perp = 0.001;
    tol.para = 0.999;

    /* Associate the face geometry. */
    fu = BU_LIST_FIRST( faceuse, &s->fu_hd );
    if (nmg_loop_plane_area(BU_LIST_FIRST(loopuse, &fu->lu_hd), pl) < 0.0)
	return -1;
    else
	nmg_face_g( fu, pl );

    if (!NEAR_ZERO(MAGNITUDE(Ext), 0.001))
	nmg_extrude_face(BU_LIST_FIRST(faceuse, &s->fu_hd), Ext, vlfree, &tol);

    nmg_region_a(r, &tol);	/* Calculate geometry for region and shell. */

    nmg_fix_normals( s, vlfree, &tol ); /* insure that faces have outward pointing normals */

    create_brlcad_db(fpout, m, reg_name, grp_name);

    return 0;
}

/*
 *	Convert an ascii description of an nmg to an actual nmg.
 *	(This should be done with perplex and lemon.)
 */
static void
descr_to_nmg(struct shell *s, FILE *fp, fastf_t *Ext)
    /* NMG shell to add loops to. */
    /* File pointer for ascii nmg file. */
    /* Extrusion vector. */
{
#define MAXV	10000
#define TOKEN_LEN 80
    char token[TOKEN_LEN+1] = {0};	/* Token read from ascii nmg file. */
    double x, y, z;	/* Coordinates of a vertex. */
    int	dir = OT_NONE;	/* Direction of face. */
    int	i,
	lu_verts[MAXV] = {0},	/* Vertex names making up loop. */
	n,		/* Number of vertices so far in loop. */
	status,		/* Set to EOF when finished ascii file. */
	vert_num;	/* Current vertex in ascii file. */
    fastf_t	pts[3*MAXV] = {(fastf_t)0};	/* Points in current loop. */
    struct faceuse *fu;	/* Face created. */
    struct vertex	*cur_loop[MAXV], /* Vertices in current loop. */
			*verts[MAXV];	/* Vertices in all loops. */

    n = 0;			/* No vertices read in yet. */
    fu = NULL;		/* Face to be created elsewhere. */
    for (i = 0; i < MAXV; i++) {
	cur_loop[i] = NULL;
	verts[i] = NULL;
    }

    status = fscanf(fp, CPP_SCAN(TOKEN_LEN), token);	/* Get 1st token. */
    if (status == EOF)
	bu_exit(EXIT_FAILURE, "asc-nmg: failed to get first token\n");

    do {
	switch (token[0]) {
	    case 'e':		/* Extrude face. */
		status = fscanf(fp, CPP_SCAN(TOKEN_LEN), token);
		switch (token[0]) {
		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
		    case '.':
		    case '+':
		    case '-':
			/* Get x value of vector. */
			x = atof(token);
			if (fscanf(fp, "%lf%lf", &y, &z) != 2)
			    bu_exit(EXIT_FAILURE, "descr_to_nmg: messed up vector\n");
			VSET(Ext, x, y, z);

			/* Get token for next trip through loop. */
			status = fscanf(fp, CPP_SCAN(TOKEN_LEN), token);
			break;
		}
		break;
	    case 'l':		/* Start new loop. */
		/* Make a loop with vertices previous to this 'l'. */
		if (n) {
		    for (i = 0; i < n; i++)
			if (lu_verts[i] >= 0)
			    cur_loop[i] = verts[lu_verts[i]];
			else /* Reuse of a vertex. */
			    cur_loop[i] = NULL;
		    fu = nmg_add_loop_to_face(s, fu, cur_loop, n,
			    dir);
		    /* Associate geometry with vertices. */
		    for (i = 0; i < n; i++) {
			if (lu_verts[i] >= 0 && !verts[lu_verts[i]]) {
			    nmg_vertex_gv( cur_loop[i],
				    &pts[3*lu_verts[i]]);
			    verts[lu_verts[i]] =
				cur_loop[i];
			}
		    }
		    /* Take care of reused vertices. */
		    for (i = 0; i < n; i++)
			if (lu_verts[i] < 0)
			    nmg_jv(verts[-lu_verts[i]], cur_loop[i]);
		    n = 0;
		}
		status = fscanf(fp, CPP_SCAN(TOKEN_LEN), token);

		switch (token[0]) {
		    case 'h':	/* Is it cw or ccw? */
			if (BU_STR_EQUAL(token, "hole"))
			    dir = OT_OPPOSITE;
			else
			    bu_exit(EXIT_FAILURE, "descr_to_nmg: expected \"hole\"\n");
			/* Get token for next trip through loop. */
			status = fscanf(fp, CPP_SCAN(TOKEN_LEN), token);
			break;

		    default:
			dir = OT_SAME;
			break;
		}
		break;

	    case 'v':		/* Vertex in current loop. */
		if (token[1] == '\0')
		    bu_exit(EXIT_FAILURE, "descr_to_nmg: vertices must be numbered.\n");
		vert_num = atoi(token+1);
		if (vert_num < 0 || vert_num >= MAXV) {
		    bu_log("Vertex number out of bounds: %d\nAborting\n", vert_num);
		    return;
		}
		status = fscanf(fp, CPP_SCAN(TOKEN_LEN), token);
		switch (token[0]) {
		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
		    case '.':
		    case '+':
		    case '-':
			/* Get coordinates of vertex. */
			x = atof(token);
			if (fscanf(fp, "%lf%lf", &y, &z) != 2)
			    bu_exit(EXIT_FAILURE, "descr_to_nmg: messed up vertex\n");
			/* Save vertex with others in current loop. */
			pts[3*vert_num] = x;
			pts[3*vert_num+1] = y;
			pts[3*vert_num+2] = z;
			/* Save vertex number. */
			lu_verts[n] = vert_num;
			if (++n >= MAXV - 1)
			    bu_exit(EXIT_FAILURE, "descr_to_nmg: too many points in loop\n");
			/* Get token for next trip through loop. */
			status = fscanf(fp, CPP_SCAN(TOKEN_LEN), token);
			break;

		    default:
			/* Use negative vert number to mark vertex as being reused. */
			lu_verts[n] = -vert_num;
			if (++n > MAXV - 1)
			    bu_exit(EXIT_FAILURE, "descr_to_nmg: too many points in loop\n");
			break;
		}
		break;

	    default:
		bu_exit(1, "descr_to_nmg: unexpected token \"%s\"\n", token);
		break;
	}
    } while (status != EOF);

    /* Make a loop with vertices previous to this 'l'. */
    if (n) {
	for (i = 0; i < n; i++)
	    if (lu_verts[i] >= 0)
		cur_loop[i] = verts[lu_verts[i]];
	    else /* Reuse of a vertex. */
		cur_loop[i] = NULL;
	nmg_add_loop_to_face(s, fu, cur_loop, n, dir);

	/* Associate geometry with vertices. */
	for (i = 0; i < n; i++) {
	    if (lu_verts[i] >= 0 && !verts[lu_verts[i]]) {
		nmg_vertex_gv( cur_loop[i],
			&pts[3*lu_verts[i]]);
		verts[lu_verts[i]] =
		    cur_loop[i];
	    }
	}
	/* Take care of reused vertices. */
	for (i = 0; i < n; i++)
	    if (lu_verts[i] < 0)
		nmg_jv(verts[-lu_verts[i]], cur_loop[i]);
    }
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
