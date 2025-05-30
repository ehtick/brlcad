/*                       M A C - P I X . C
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
 */
/** @file util/mac-pix.c
 *
 * Read MacPaint document and output pix(5) or bw(5) raster image
 *
 * The Macintosh bitmap file has a 512 byte header which is
 * unimportant for this application, followed by a run-length-encoded
 * image.
 *
 * The image is stored as a sequence of bits, proceeding from the
 * upper right corner down to the lower right corner, and then
 * advancing left one column.  This unusual format
 * somewhat complicates matters here.
 *
 * Within each byte, bits are extracted MSB to LSB;  these bits go down
 * the page.
 *
 * Because two files are being processed, the "file_*" things refer
 * to the input file, and the "scr_*" things refer to the output file.
 *
 */

#include "common.h"

#include <stdlib.h>
#include "bio.h"

#include "bu/app.h"
#include "bu/getopt.h"
#include "bu/exit.h"


#define MAC_HEIGHT 576	/* input height (y), in bits */
#define MAC_WIDTH 720	/* input width (x), in bits */

struct macpaint_hdr {
    long version;		/* 4 bytes */
    char patterns[38*8];		/* 304 bytes */
    char spare[204];
} hdr;

unsigned char pix[MAC_HEIGHT*MAC_WIDTH];

unsigned char color[3] = { 0xFF, 0xFF, 0xFF };
unsigned char black[3*2048];

int file_height = MAC_HEIGHT;	/* generally constant */
int file_width = MAC_WIDTH;
int file_xoff = 0;
int file_yoff = 0;
int scr_width = 1024;	/* If this and scr_height are later found to be zero,
			 * they assume the values of file_width and file_height .
			 */
int scr_height = 1024;
int scr_xoff = 0;
int scr_yoff = 0;

int bwflag = 0;
char hyphen[] = "-";
char *file_name;
FILE *infp;

static char usage[] = "\
Usage: mac-pix [-c -l -b]\n\
	[-s squareMacsize] [-w Mac_width] [-n Mac_height]\n\
	[-x Mac_xoff] [-y Mac_yoff] [-X outp_xoff] [-Y outp_yoff]\n\
	[-S squareoutpsize] [-W outp_width] [-N outp_height]\n\
	[-C r/g/b] [file.mac]\n\
       (standard output must be redirected)\n";

int
get_args(int argc, char **argv)
{
    int c;

    while ((c = bu_getopt(argc, argv, "clbs:w:n:x:y:X:Y:S:W:N:C:h?")) != -1) {
	switch (c) {
	    case 'c':
		/* Center in output */
		scr_xoff = (scr_width-file_width)/2;
		scr_yoff = (scr_height-file_height)/2;
		break;
	    case 'b':
		bwflag = 1;
		break;
	    case 'l':
		/* "low"-res output -- track file size */
		scr_height = scr_width = 0;
		break;
	    case 's':
		/* square file size */
		file_height = file_width = atoi(bu_optarg);
		break;
	    case 'w':
		file_width = atoi(bu_optarg);
		break;
	    case 'n':
		file_height = atoi(bu_optarg);
		break;
	    case 'N':
		scr_height = atoi(bu_optarg);
		break;
	    case 'x':
		file_xoff = atoi(bu_optarg);
		break;
	    case 'X':
		scr_xoff += atoi(bu_optarg);
		break;
	    case 'y':
		file_yoff = atoi(bu_optarg);
		break;
	    case 'Y':
		scr_yoff += atoi(bu_optarg);
		break;
	    case 'S':
		scr_height = scr_width = atoi(bu_optarg);
		break;
	    case 'W':
		scr_width = atoi(bu_optarg);
		break;
	    case 'C':
		{
		    char *cp = bu_optarg;
		    unsigned char *conp
			= (unsigned char *)color;

		    /* premature null => atoi gives zeros */
		    for (c=0; c < 3; c++) {
			*conp++ = atoi(cp);
			while (*cp && *cp++ != '/')
			    ;
		    }
		}
		break;

	    default:		/* '?' or 'h' will get you here */
		return 0;
	}

	/* enforce limits */
	if (file_width > MAC_WIDTH)
	    file_width = MAC_WIDTH;
	if (file_height > MAC_HEIGHT)
	    file_height = MAC_HEIGHT;
	if (file_xoff < 0 || file_xoff >= MAC_WIDTH)
	    file_xoff = 0;
	if (file_yoff < 0 || file_yoff >= MAC_HEIGHT)
	    file_yoff = 0;
    }

    if (bu_optind >= argc) {
	if (isatty(fileno(stdin)))
	    return 0;
	file_name = hyphen;
	infp = stdin;
    } else {
	file_name = argv[bu_optind];
	if ((infp = fopen(file_name, "rb")) == NULL) {
	    fprintf(stderr,
		    "mac-pix: cannot open \"%s\" for reading\n",
		    file_name);
	    return 0;
	}
    }

    if (argc > ++bu_optind)
	fprintf(stderr, "mac-pix: excess argument(s) ignored\n");

    return 1;		/* OK */
}


int
getbits(FILE *fp)
{
    static int count, rep, chr;
    int c;

    if (rep) {
	rep--;
	return chr;
    }
    if (count) {
	count--;
	return getc(fp);
    }
    c = getc(fp);
    if (c & 0x80) {
	/* repeated character count */
	rep = 0x100 - c;	/* byte length 2's comp + 1 */
	/* allow for this call */
	chr = getc(fp);		/* character to repeat */
	return chr;
    } else {
	count = c;		/* already counted this char */
	return getc(fp);
    }
}


int
main(int argc, char **argv)
{
    int c;
    int x, y;
    int x1, x2, x3;		/* x zone widths */
    int y1, y2, y3;		/* y zone widths */
    int first_x;		/* x: first pixel to be output in pix[] */
    int first_y;		/* y: first pixel to be output in pix[] */
    size_t ret;

    bu_setprogname(argv[0]);

    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);

    if (!get_args(argc, argv) || isatty(fileno(stdout))) {
	(void)fputs(usage, stderr);
	bu_exit (1, NULL);
    }

    /* If screen size was not set, track the file size */
    if (scr_width == 0)
	scr_width = file_width;
    if (scr_height == 0)
	scr_height = file_height;

    ret = fread((char *)&hdr, sizeof(hdr), 1, infp);
    if (ret == 0) {
	perror("fread");
	bu_exit (1, NULL);
    }

    /* x and y are in terms of 1st quadrant coordinates */
    /* Very first bit input is in upper right of screen */
    x = file_width-1;
    y = file_height-1;

    while ((c = getbits(infp)) != EOF) {
	int mask;
	unsigned char *cp;

	cp = &pix[(file_width*y)+x];

	for (mask = 0x80; mask; mask >>= 1) {
	    if (c & mask)
		*cp = 0xFF;
	    else
		*cp = 0;
	    cp -= file_width;
	}

	y -= 8;
	if (y < 0) {
	    y = file_height-1;
	    if (--x < 0)
		break;
	}
    }
    fclose(infp);

    /*
     * There are potentially 9 regions in the output file, only one
     * will contain actual image.  x1 is the width of the left border
     * for each scanline, x2 is the width of the image area,
     * with the first pixel being first_x, and x3 being the width
     * of the right border.
     * Computations in y are similar.
     */
    first_y = file_yoff;		/* always >= 0 */
    y1 = scr_yoff;
    if (y1 < 0) {
	y1 = 0;
	first_y += -scr_yoff;
    }
    y2 = file_height - first_y;
    if (y1 + y2 > scr_height)
	y2 = scr_height - y1;
    if (first_y >= file_height)
	y2 = 0;
    y3 = scr_height - y1 - y2;

    first_x = file_xoff;
    x1 = scr_xoff;
    if (x1 < 0) {
	x1 = 0;
	first_x += -scr_xoff;
    }
    x2 = file_width - first_x;
    if (x1 + x2 > scr_width)
	x2 = scr_width - x1;
    if (first_x >= file_width)
	x2 = 0;
    x3 = scr_width - x1 - x2;

    if (bwflag) {
	for (y = 0; y < y1; y++) {
	    ret = fwrite(black, scr_width, 1, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	}
	for (y = 0; y < y2; y++) {
	    ret = fwrite(black, x1, 1, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	    ret = fwrite(&pix[(file_width*(y+first_y))+first_x], x2, 1, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }

	    ret = fwrite(black, x3, 1, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	}
	for (y = 0; y < y3; y++) {
	    ret = fwrite(black, scr_width, 1, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	}
    } else {
	for (y = 0; y < y1; y++) {
	    ret = fwrite(black, scr_width, 3, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	}
	for (y = 0; y < y2; y++) {
	    unsigned char *cp;

	    ret = fwrite(black, x1, 3, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	    cp = &pix[(file_width*(y+first_y))+first_x];
	    for (x = 0; x < x2; x++) {
		if (*cp++)
		    ret = fwrite(color, 3, 1, stdout);
		else
		    ret = fwrite(black, 3, 1, stdout);

		if (ret == 0) {
		    perror("fwrite");
		    break;
		}
	    }
	    ret = fwrite(black, x3, 3, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	}
	for (y = 0; y < y3; y++) {
	    ret = fwrite(black, scr_width, 3, stdout);
	    if (ret == 0) {
		perror("fwrite");
		break;
	    }
	}
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
