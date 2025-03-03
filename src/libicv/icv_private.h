/*                  I C V _ P R I V A T E . H
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
#include "bu/mime.h"
#include "bio.h" /* for O_BINARY */
#include "icv.h"

#ifndef ICV_PRIVATE_H
#define ICV_PRIVATE_H

/* defined in bw.c */
extern icv_image_t *bw_read(FILE *fp, size_t width, size_t height);
extern int bw_write(icv_image_t *bif, FILE *fp);

/* defined in pix.c */
extern icv_image_t *pix_read(FILE *fp, size_t width, size_t height);
extern int pix_write(icv_image_t *bif, FILE *fp);

/* defined in dpix.c */
extern icv_image_t *dpix_read(FILE *fp, size_t width, size_t height);
extern int dpix_write(icv_image_t *bif, FILE *fp);

/* defined in png.c */
extern icv_image_t* png_read(FILE *fp);
extern int png_write(icv_image_t *bif, FILE *fp);

/* defined in ppm.c */
extern icv_image_t* ppm_read(FILE *fp);
extern int ppm_write(icv_image_t *bif, FILE *fp);

/* defined in rle.c */
extern icv_image_t* rle_read(FILE *fp);
extern int rle_write(icv_image_t *bif, FILE *fp);

#endif /* ICV_PRIVATE_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
