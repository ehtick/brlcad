/*                      F I L E F O R M A T . C
 * BRL-CAD
 *
 * Copyright (c) 2007-2025 United States Government as represented by
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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>	/* for file mode info in WRMODE */
#include "png.h"

#include "bio.h"

#include "bu/str.h"
#include "bu/file.h"
#include "bu/log.h"
#include "bu/malloc.h"
#include "bu/mime.h"
#include "bu/path.h"
#include "bn.h"
#include "vmath.h"
#include "icv_private.h"

/* private functions */

/*
 * Attempt to guess the file type. Understands ImageMagick style
 * FMT:filename as being preferred, but will attempt to guess based on
 * extension as well.
 */
static bu_mime_image_t
icv_guess_file_format(const char *filename, struct bu_vls *trimmedname)
{
    // If we have no filename, there's nothing to go on
    if (!filename)
	return BU_MIME_IMAGE_PIX;

    /* look for the FMT: header */
#define CMP(name) if (!bu_strncmp(filename, #name":", strlen(#name))) {if (trimmedname) bu_vls_sprintf(trimmedname, "%s", filename+strlen(#name)+1); return BU_MIME_IMAGE_##name; }
    CMP(PIX);
    CMP(PNG);
    CMP(PPM);
    CMP(BMP);
    CMP(BW);
    CMP(DPIX)
#undef CMP

    /* no format header found, copy the name as it is */
    if (trimmedname)
        bu_vls_sprintf(trimmedname, filename, BUFSIZ);

    /* and guess based on extension */
#define CMP(name, ext) if (!bu_strncmp(filename+strlen(filename)-strlen(#name)-1, "."#ext, strlen(#name)+1)) return BU_MIME_IMAGE_##name;
    CMP(PIX, pix);
    CMP(PNG, png);
    CMP(PPM, ppm);
    CMP(BMP, bmp);
    CMP(BW, bw);
    CMP(DPIX, dpix);
#undef CMP
    /* defaulting to PIX */
    return BU_MIME_IMAGE_PIX;
}

/* begin public functions */

icv_image_t *
icv_read(const char *filename, bu_mime_image_t format, size_t width, size_t height)
{
    if (format == BU_MIME_IMAGE_AUTO)
	format = icv_guess_file_format(filename, NULL);

    FILE *fp = (!filename) ? stdin : fopen(filename, "rb");
    if (!fp) {
	bu_log("ERROR: Cannot open file %s for reading\n", filename);
	return NULL;
    }
    if (!filename)
	setmode(fileno(fp), O_BINARY);

    icv_image_t *oimg = NULL;
    switch (format) {
	case BU_MIME_IMAGE_PNG:
	    oimg = png_read(fp);
	    break;
	case BU_MIME_IMAGE_PIX:
	    oimg = pix_read(fp, width, height);
	    break;
	case BU_MIME_IMAGE_BW :
	    oimg = bw_read(fp, width, height);
	    break;
	case BU_MIME_IMAGE_DPIX :
	    oimg = dpix_read(fp, width, height);
	    break;
	case BU_MIME_IMAGE_PPM :
	    oimg = ppm_read(fp);
	    break;
	case BU_MIME_IMAGE_RLE :
	    oimg = rle_read(fp);
	    break;
	default:
	    bu_log("icv_read not implemented for this format\n");
	    fclose(fp);
	    return NULL;
    }

    fflush(fp);
    fclose(fp);
    return oimg;
}


int
icv_write(icv_image_t *bif, const char *filename, bu_mime_image_t format)
{
    int ret = 0;
    struct bu_vls ofilename = BU_VLS_INIT_ZERO;
    const char *ofname = filename;

    if (format == BU_MIME_IMAGE_AUTO) {
	format = icv_guess_file_format(filename, &ofilename);
	if (filename)
	    ofname = bu_vls_cstr(&ofilename);
    }

    ICV_IMAGE_VAL_INT(bif);

    FILE *fp = (ofname==NULL) ? stdout : fopen(ofname, "wb");
    if (UNLIKELY(fp==NULL)) {
	perror("fopen");
	bu_log("ERROR: icv_write failed to get a FILE pointer for %s\n", filename);
	return BRLCAD_ERROR;
    }

    switch (format) {
	/* case BU_MIME_IMAGE_BMP:
	   return bmp_write(bif, ofname); */
	case BU_MIME_IMAGE_PPM:
	    ret = ppm_write(bif, fp);
	    break;
	case BU_MIME_IMAGE_PNG:
	    ret = png_write(bif, fp);
	    break;
	case BU_MIME_IMAGE_PIX:
	    ret = pix_write(bif, fp);
	    break;
	case BU_MIME_IMAGE_BW:
	    ret = bw_write(bif, fp);
	    break;
	case BU_MIME_IMAGE_DPIX :
	    ret = dpix_write(bif, fp);
	    break;
	case BU_MIME_IMAGE_RLE :
	    ret = rle_write(bif, fp);
	    break;
	default:
	    ret = pix_write(bif, fp);
    }

    fflush(fp);
    fclose(fp);

    bu_vls_free(&ofilename);
    return ret;
}


int
icv_writeline(icv_image_t *bif, size_t y, void *data, ICV_DATA type)
{
    double *dst;
    size_t width_size;
    unsigned char *p=NULL;

    if (bif == NULL || data == NULL)
	return -1;

    ICV_IMAGE_VAL_INT(bif);

    if (y > bif->height)
        return -1;

    width_size = (size_t) bif->width*bif->channels;
    dst = bif->data + width_size*y;

    if (type == ICV_DATA_UCHAR) {
	p = (unsigned char *)data;
	for (; width_size > 0; width_size--) {
		*dst = ICV_CONV_8BIT(*p);
		p++;
		dst++;
	}
    } else
	memcpy(dst, data, width_size*sizeof(double));


    return 0;
}


int
icv_writepixel(icv_image_t *bif, size_t x, size_t y, double *data)
{
    double *dst;

    ICV_IMAGE_VAL_INT(bif);

    if (x > bif->width)
        return -1;

    if (y > bif->height)
        return -1;

    if (data == NULL)
        return -1;

    dst = bif->data + (y*bif->width + x)*bif->channels;

    /* can copy float to double also double to double */
    VMOVEN(dst, data, bif->channels);
    return 0;
}


icv_image_t *
icv_create(size_t width, size_t height, ICV_COLOR_SPACE color_space)
{
    icv_image_t *bif;
    BU_ALLOC(bif, struct icv_image);
    bif->width = width;
    bif->height = height;
    bif->color_space = color_space;
    bif->alpha_channel = 0;
    bif->magic = ICV_IMAGE_MAGIC;
    switch (color_space) {
	case ICV_COLOR_SPACE_RGB :
	    /* Add all the other three channel images here (e.g. HSV, YCbCr, etc.) */
	    bif->data = (double *) bu_malloc(bif->height*bif->width*3*sizeof(double), "Image Data");
	    bif->channels = 3;
	    break;
	case ICV_COLOR_SPACE_GRAY :
	    bif->data = (double *) bu_malloc(bif->height*bif->width*1*sizeof(double), "Image Data");
	    bif->channels = 1;
	    break;
	default :
	    bu_exit(1, "icv_create_image : Color Space Not Defined");
	    break;
    }
    return icv_zero(bif);
}


icv_image_t *
icv_zero(icv_image_t *bif)
{
    double *data;
    size_t size, i;

    ICV_IMAGE_VAL_PTR(bif);

    data = bif->data;
    size = bif->width * bif->height * bif->channels;
    for (i = 0; i < size; i++)
	*data++ = 0;

    return bif;
}


int
icv_destroy(icv_image_t *bif)
{
    ICV_IMAGE_VAL_INT(bif);

    bu_free(bif->data, "Image Data");
    bu_free(bif, "ICV IMAGE Structure");
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
