/*                        R P A T C H . C
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
 */
/** @file rpatch.c
 *
 * Front end to patch.
 *
 * This pre-processor program alters the data file format for use by
 * the main conversion program.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "bio.h"

#include "bu/app.h"
#include "bu/exit.h"
#include "bu/getopt.h"
#include "bu/log.h"


#define MAXLINELEN 256

static const char *usage="Usage:\n\trpatch [-D] [-3] < fastgen_input_file > file.rp\n\
	where -D means that type 3 components are donuts (rather than triangles)\n\
	and -3 indicates that the input is in FASTGEN3 format\n";

static int fast3;

double
get_ftn_float(char *str, unsigned int start_col, char *format)
{
    char *ptr = format;
    char tmp_str[MAXLINELEN] = {0};
    int width = 0;
    int precision = 0;
    int leading_spaces=1;
    int i = 0;

    /* Check that format is legal for floating point or double */
    if (*ptr != 'F' && *ptr != 'f' && *ptr != 'E' && *ptr != 'e' && *ptr != 'D' && *ptr != 'd') {
	fprintf(stderr, "Get_ftn_float(str=%s\n, start_col=%d, format=%s)\n",
		str, start_col, ptr);
	fprintf(stderr, "\tformat must be F, E, or D type\n");
	bu_exit(1, NULL);
    }

    /* if start column is beyond end of input string, return zero */
    if (start_col >= strlen(str))
	return (double)0.0;

    /* get width from format spec */
    ptr++;
    width = atoi(ptr);

    /* make sure input string will fit in tmp_str (minus trailing NULL and a decimal point) */
    if (width > MAXLINELEN-2) {
	fprintf(stderr, "Get_ftn_float(str=%s\n, start_col=%d, format=%s)\n",
		str, start_col, ptr);
	fprintf(stderr, "\tfield width (%d) in format is too large. Max allowed is %d\n",
		width, MAXLINELEN-2);
	bu_exit(1, NULL);
    }

    /* copy the input string to tmp_str, converting
     * embedded blanks to zeros and 'D' or 'd' to 'e'
     */
    for (i=0; i<width; i++) {
	if (isspace((int)str[start_col+i])) {
	    if (leading_spaces)
		tmp_str[i] = ' ';
	    else
		tmp_str[i] = '0';
	} else if (str[start_col+i] == 'D' || str[start_col+i] == 'd') {
	    leading_spaces = 0;
	    tmp_str[i] = 'e';
	} else {
	    leading_spaces = 0;
	    tmp_str[i] = str[start_col+i];
	}
    }
    tmp_str[width] = '\0';

    /* get precision from format spec */
    ptr = strchr(ptr, '.');
    if (ptr)
	precision = atoi(++ptr);
    else
	precision = 0;


    /* if there is a decimal point, let atof handle the rest (including exponent) */
    if (!strchr(tmp_str, '.') && precision > 0) {
	/* insert a decimal point where needed */
	for (i=0; i<precision; i++)
	    tmp_str[width-i] = tmp_str[width-i-1];
	tmp_str[width-precision] = '.';
	tmp_str[width+1] = '\0';

	/* and atof can handle it from here */
    }

    return atof(tmp_str);
}


int
get_ftn_int(char *str, unsigned int start_col, char *format)
{
    char *ptr;
    char tmp_str[MAXLINELEN];
    int leading_spaces=1;
    int width;
    int i;

    /* check that format id for an integer */
    ptr = format;
    if (*ptr != 'I' && *ptr != 'i') {
	fprintf(stderr, "Get_ftn_int(str=%s\n, start_col=%d, format=%s)\n",
		str, start_col, ptr);
	fprintf(stderr, "\tformat must be I type\n");
	bu_exit(1, NULL);
    }

    /* if start column is beyond end of input string, return zero */
    if (start_col >= strlen(str))
	return 0;

    /* get width from format spec */
    ptr++;
    width = atoi(ptr);

    /* make sure input string will fit in tmp_str */
    if (width > MAXLINELEN-1) {
	fprintf(stderr, "Get_ftn_int(str=%s\n, start_col=%d, format=%s)\n",
		str, start_col, ptr);
	fprintf(stderr, "\tfield width (%d) in format is too large. Max allowed is %d\n",
		width, MAXLINELEN-1);
	bu_exit(1, NULL);
    }

    /* copy the input string to tmp_str, converting
     * embedded blanks to zeros */
    for (i=0; i<width; i++) {
	if (isspace((int)str[start_col+i])) {
	    if (leading_spaces)
		tmp_str[i] = ' ';
	    else
		tmp_str[i] = '0';
	} else {
	    leading_spaces = 0;
	    tmp_str[i] = str[start_col+i];
	}
    }
    tmp_str[width] = '\0';

    return atoi(tmp_str);
}


int
main(int argc, char **argv)
{
    char line[MAXLINELEN];
    float x, y, z, hold, work;
    char minus;
    int ity, ity1, ico, isq[8], m, n, cc, tmp;
    int i;
    int type3_is_donut=0;
    int c;

    bu_setprogname(argv[0]);

    fast3 = 0;
    if (argc > 2) {
	bu_exit(1, "%s", usage);
    }

    /* Get command line arguments. */
    while ((c = bu_getopt(argc, argv, "D3")) != -1) {
	switch (c) {
	    case 'D':  /* donuts */
		type3_is_donut = 1;
		break;
	    case '3':	/* FASTGEN3 format?? */
		fast3 = 1;
		break;
	    case '?':
		bu_exit(1, "%s", usage);
		break;
	    default:
		bu_exit(1, "Illegal option (%c)\n%s", c, usage);
	}
    }

    while (bu_fgets(line, sizeof(line), stdin)) {
	if (strlen(line) <= 1)
	    continue;
	line[strlen(line)-1] = '\0';	/* eliminate \n */

	if (fast3) {
	    x = get_ftn_float(line, 0, "f10.3");
	    y = get_ftn_float(line, 10, "f10.3");
	    z = get_ftn_float(line, 20, "f10.3");
	    tmp = get_ftn_int(line, 30, "i6");
	    cc = get_ftn_int(line, 36, "i4");
	    isq[0] = get_ftn_int(line, 40, "i10");

	    for (i=1; i<8; i++)
		isq[i] = get_ftn_int(line, 50 + (i-1)*4, "i4");

	    m = get_ftn_int(line, 74, "i3");
	    n = get_ftn_int(line, 77, "i3");
	} else {
	    x = get_ftn_float(line, 0, "f8.3");
	    y = get_ftn_float(line, 8, "f8.3");
	    z = get_ftn_float(line, 16, "f9.3");
	    tmp = get_ftn_int(line, 25, "i6");
	    cc = get_ftn_int(line, 31, "i4");
	    isq[0] = get_ftn_int(line, 35, "i11");

	    for (i=1; i<8; i++)
		isq[i] = get_ftn_int(line, 46 + (i-1)*4, "i4");

	    m = get_ftn_int(line, 74, "i3");
	    n = get_ftn_int(line, 77, "i3");
	}

	/* get plate mode flag */
	minus = '+';
	if (tmp < 0) {
	    tmp = (-tmp);
	    minus = '-';
	}

	/* get solid type */
	hold = (float)tmp/10000.0;
	work = hold * 10.0;
	ity = work;
	hold = work - ity;
	if (ity == 4)
	    ity = 8;
	else if (ity == 3 && type3_is_donut)
	    ity = 4;

	/* get thickness */
	work = hold * 100.0;
	ico = work;
	hold = work - ico;

	/* get space code */
	work = hold * 10.0;
	ity1 = work;

	/* write output */
	printf("%8.3f %8.3f %9.3f %c %2d %2d %1d %4d %11d ",
	       x, y, z, minus, ity, ico, ity1, cc, isq[0]);
	for (i=1; i<8; i++)
	    printf("%5d", isq[i]);
	printf(" %3d %3d\n", m, n);

    }
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
