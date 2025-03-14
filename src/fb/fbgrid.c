/*                        F B G R I D . C
 * BRL-CAD
 *
 * Copyright (c) 1986-2025 United States Government as represented by
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
/** @file fbgrid.c
 *
 */

#include "common.h"

#include <stdlib.h>

#include "bio.h"

#include "bu/app.h"
#include "bu/color.h"
#include "bu/getopt.h"
#include "bu/exit.h"
#include "dm.h"


static unsigned char *white_line, *grey_line, *dark_line;
static struct fb *fbp;
static char *framebuffer = NULL;

#define OLD 0
#define BINARY 1
#define DECIMAL 2

static int fbwidth = 0;
static int fbheight = 0;
static int flavor = DECIMAL;
static int clear = 0;

void grid(struct fb *fbiop, unsigned char *line, int spacing), oldflavor(void);

static char usage[] = "\
Usage: fbgrid [-c] [-b | -d | -o] [-F framebuffer]\n\
	[-S squaresize] [-W width] [-N height]\n";

int
get_args(int argc, char **argv)
{
    int c;

    while ((c = bu_getopt(argc, argv, "cbdoF:s:w:n:S:W:N:h?")) != -1) {
	switch (c) {
	    case 'c':
		clear = 1;
		break;
	    case 'b':
		flavor = BINARY;
		break;
	    case 'd':
		flavor = DECIMAL;
		break;
	    case 'o':
		flavor = OLD;
		break;
	    case 'F':
		framebuffer = bu_optarg;
		break;
	    case 'S':
	    case 's':
		/* square size */
		fbheight = fbwidth = atoi(bu_optarg);
		break;
	    case 'W':
	    case 'w':
		fbwidth = atoi(bu_optarg);
		break;
	    case 'N':
	    case 'n':
		fbheight = atoi(bu_optarg);
		break;

	    default:		/* '?' */
		return 0;
	}
    }

    if (argc == 1 && isatty(fileno(stdin)) && isatty(fileno(stdout)))
	return 0;

    if (argc > ++bu_optind)
	fprintf(stderr, "fbgrid: excess argument(s) ignored\n");

    return 1;		/* OK */
}


int
main(int argc, char **argv)
{
    int i;

    bu_setprogname(argv[0]);

    if (!get_args(argc, argv)) {
	(void)fputs(usage, stderr);
	bu_exit(1, NULL);
    }

    if (flavor == OLD)
	oldflavor();	/* exits */

    if ((fbp = fb_open(framebuffer, fbwidth, fbheight)) == NULL)
	bu_exit(2, NULL);

    fbwidth = fb_getwidth(fbp);
    fbheight = fb_getheight(fbp);

    /* Initialize the color lines */
    white_line = (unsigned char *)malloc(fbwidth * sizeof(RGBpixel));
    grey_line  = (unsigned char *)malloc(fbwidth * sizeof(RGBpixel));
    dark_line  = (unsigned char *)malloc(fbwidth * sizeof(RGBpixel));
    for (i = 0; i < fbwidth; i++) {
	white_line[3*i+RED] = white_line[3*i+GRN] = white_line[3*i+BLU] = 255;
	grey_line[3*i+RED] = grey_line[3*i+GRN] = grey_line[3*i+BLU] = 128;
	dark_line[3*i+RED] = dark_line[3*i+GRN] = dark_line[3*i+BLU] = 64;
    }

    if (clear)
	fb_clear(fbp, PIXEL_NULL);

    if (flavor == BINARY) {
	/* Dark lines every 8 */
	grid(fbp, dark_line, 8);
	/* Grey lines every 64 */
	grid(fbp, grey_line, 64);
	/* White line every 128 */
	grid(fbp, white_line, 128);
    } else {
	/* DECIMAL */
	/* Dark lines every 10 */
	grid(fbp, dark_line, 10);
	/* Grey lines every 50 */
	grid(fbp, grey_line, 50);
	/* White line every 100 */
	grid(fbp, white_line, 100);
    }

    fb_close(fbp);
    return 0;
}


void
grid(struct fb *fbiop, unsigned char *line, int spacing)
{
    int x, y;

    for (y = 0; y < fbheight; y += spacing)
	fb_write(fbiop, 0, y, line, fbwidth);
    for (x = 0; x < fbwidth; x += spacing) {
	fb_writerect(fbiop, x, 0, 1, fbheight, line);
    }
}


void
oldflavor(void)
{
    struct fb *fbiop;
    int x, y;
    int middle;
    int mask;
    int fb_sz;
    static RGBpixel black, white, red;

    fbiop = fb_open(NULL, fbwidth, fbheight);
    if (fbiop == NULL) {
	bu_exit(1, NULL);
    }

    fb_sz = fb_getwidth(fbp);
    white[RED] = white[GRN] = white[BLU] = 255;
    black[RED] = black[GRN] = black[BLU] = 0;
    red[RED] = 255;
    middle = fb_sz/2;
    fb_ioinit(fbp);
    if (fb_sz <= 512)
	mask = 0x7;
    else
	mask = 0xf;

    for (y = fb_sz-1; y >= 0; y--) {
	for (x = 0; x < fb_sz; x++) {
	    if (x == y || x == fb_sz - y) {
		fb_wpixel(fbp, white);
	    } else
		if (x == middle || y == middle) {
		    fb_wpixel(fbp, red);
		} else
		    if ((x & mask) && (y & mask)) {
			fb_wpixel(fbp, black);
		    } else {
			fb_wpixel(fbp, white);
		    }
	}
    }
    fb_close(fbp);
    bu_exit(0, NULL);
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
