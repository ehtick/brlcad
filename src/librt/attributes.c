/*                    A T T R I B U T E S . C
 * BRL-CAD
 *
 * Copyright (c) 2010-2025 United States Government as represented by
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

#include <string.h>

#include "bu/debug.h"
#include "raytrace.h"


int
db5_import_attributes(struct bu_attribute_value_set *avs, const struct bu_external *ap)
{
    const char *cp;
    const char *ep;
    int count = 0;
#if defined(USE_BINARY_ATTRIBUTES)
    int bcount = 0; /* for the subset of binary attributes */
#endif

    BU_CK_EXTERNAL(ap);

    BU_ASSERT(ap->ext_nbytes >= 4);

    /* First pass -- count number of attributes */
    cp = (const char *)ap->ext_buf;
    ep = (const char *)ap->ext_buf+ap->ext_nbytes;

    /* An empty "name" string (a pair of nul bytes) indicates end of
     * attribute list (for the original ASCII-valued attributes).
     */
    while (*cp != '\0') {
	if (cp >= ep) {
	    bu_log("db5_import_attributes() ran off end of buffer, database is probably corrupted\n");
	    return -1;
	}
	cp += strlen(cp)+1;	/* value */
	cp += strlen(cp)+1;	/* next name */
	count++;
    }
#if defined(USE_BINARY_ATTRIBUTES)
    /* Do we have binary attributes?  If so, they are after the last
     * ASCII attribute.
     */
    if (ep > (cp+1)) {
	/* Count binary attrs. */
	/* format is: <ascii name> NULL <binary length [network order, must be decoded]> <bytes...> */
	size_t abinlen;
	cp += 2; /* We are now at the first byte of the first binary attribute... */
	while (cp != ep) {
	    ++bcount;
	    cp += strlen(cp)+1;	/* name */
	    /* The next value is an unsigned integer of variable width
	     * (a_width: DB5HDR_WIDTHCODE_x) so how do we get its
	     * width?  We now have a new member of struct bu_external:
	     * 'unsigned char widcode'.  Note that the integer is in
	     * network order and must be properly decoded for the
	     * local architecture.
	     */
	    cp += db5_decode_length(&abinlen, (const unsigned char*)cp, ap->widcode);
	    /* account for the abinlen bytes */
	    cp += abinlen;
	}
	/* now cp should be at the end */
	count += bcount;
    } else {
	/* step to the end for the sanity check */
	++cp;
    }
    /* Ensure we're exactly at the end */
    BU_ASSERT(cp == ep);
#else
    /* Ensure we're exactly at the end */
    BU_ASSERT(cp+1 == ep);
#endif

    /* not really needed for AVS_ADD since bu_avs_add will
     * incrementally allocate as it needs it. but one alloc is better
     * than many in case there are many attributes.
     */
    bu_avs_init(avs, count, "db5_import_attributes");

    /* Second pass -- populate attributes. */

    /* Copy the values from the external buffer instead of using them
     * directly without copying.  This presumes ap will not get free'd
     * before we're done with the avs.
     */

    cp = (const char *)ap->ext_buf;
    while (*cp != '\0') {
	const char *name = cp;  /* name */
	cp += strlen(cp)+1; /* value */
	bu_avs_add(avs, name, cp);
	cp += strlen(cp)+1; /* next name */
    }
#if defined(USE_BINARY_ATTRIBUTES)
    /* Do we have binary attributes?  If so, they are after the last
     * ASCII attribute. */
    if (ep > (cp+1)) {
	/* Count binary attrs. */
	/* format is: <ascii name> NULL <binary length [network order, must be decoded]> <bytes...> */
	size_t abinlen;
	cp += 2; /* We are now at the first byte of the first binary
		  * attribute...
		  */
	while (cp != ep) {
	    const char *name = cp;  /* name */
	    cp += strlen(cp)+1;	/* name */
	    cp += db5_decode_length(&abinlen, (const unsigned char*)cp, ap->widcode);
	    /* now decode for the abinlen bytes */
	    decode_binary_attribute(abinlen, (const unsigned char*)cp);
	    cp += abinlen;
	}
	/* now cp should be at the end */
    } else {
	/* step to the end for the sanity check */
	++cp;
    }
    /* Ensure we're exactly at the end */
    BU_ASSERT(cp == ep);
#else
    BU_ASSERT(cp+1 == ep);
#endif

    BU_ASSERT(avs->count <= avs->max);
    BU_ASSERT((size_t)avs->count == (size_t)count);

    if (bu_debug & BU_DEBUG_AVS) {
	bu_avs_print(avs, "db5_import_attributes");
    }
    return avs->count;
}


void
db5_export_attributes(struct bu_external *ext, const struct bu_attribute_value_set *avs)
{
    size_t need = 0;
    const struct bu_attribute_value_pair *avpp;
    char *cp;
    size_t i;
    size_t len;

    BU_CK_AVS(avs);
    BU_EXTERNAL_INIT(ext);

    if (avs->count <= 0) {
	return;
    }

    if (bu_debug & BU_DEBUG_AVS) {
	bu_avs_print(avs, "db5_export_attributes");
    }

    /* First pass -- determine how much space is required */
    need = 0;
    avpp = avs->avp;
    for (i = 0; i < avs->count; i++, avpp++) {
	if (avpp->name) {
	    need += strlen(avpp->name);
	}
	need += 1; /* include room for nul byte */
	if (avpp->value) {
	    need += strlen(avpp->value);
	}
	need += 1; /* include room for nul byte */
    }
    /* include a final nul byte */
    need += 1;

    if (need <= 1) {
	/* nothing to do */
	return;
    }

    ext->ext_nbytes = need;
    ext->ext_buf = (uint8_t*)bu_calloc(1, need, "external attributes");

    /* Second pass -- store in external form */
    cp = (char *)ext->ext_buf;
    avpp = avs->avp;
    for (i = 0; i < avs->count; i++, avpp++) {
	if (avpp->name) {
	    len = strlen(avpp->name);
	    memcpy(cp, avpp->name, len);
	    cp += len;
	}
	*(cp++) = '\0'; /* pad nul byte */

	if (avpp->value) {
	    len = strlen(avpp->value);
	    memcpy(cp, avpp->value, strlen(avpp->value));
	    cp += len;
	}
	*(cp++) = '\0'; /* pad nul byte */
    }
    *(cp++) = '\0'; /* final nul byte */

    /* sanity check */
    need = cp - ((char *)ext->ext_buf);
    BU_ASSERT(need == ext->ext_nbytes);
}


int
db5_replace_attributes(struct directory *dp, struct bu_attribute_value_set *avsp, struct db_i *dbip)
{
    struct bu_external ext;
    struct db5_raw_internal raw;
    struct bu_external attr;
    struct bu_external ext2;
    int ret;

    RT_CK_DIR(dp);
    BU_CK_AVS(avsp);
    RT_CK_DBI(dbip);

    if (RT_G_DEBUG&RT_DEBUG_DB) {
	bu_log("db5_replace_attributes(%s) dbip=%p\n",
	       dp->d_namep, (void *)dbip);
	bu_avs_print(avsp, "new attributes");
    }

    if (dbip->dbi_read_only) {
	bu_log("db5_replace_attributes(%s):  READ-ONLY file\n",
	       dbip->dbi_filename);
	return -1;
    }

    BU_ASSERT(dbip->dbi_version == 5);

    if (db_get_external(&ext, dp, dbip) < 0)
	return -2;		/* FAIL */

    if (db5_get_raw_internal_ptr(&raw, ext.ext_buf) == NULL) {
	bu_log("db5_replace_attributes(%s):  import failure\n",
	       dp->d_namep);
	bu_free_external(&ext);
	return -3;
    }

    db5_export_attributes(&attr, avsp);
    BU_EXTERNAL_INIT(&ext2);
    db5_export_object3(&ext2,
		       raw.h_dli,
		       dp->d_namep,
		       raw.h_name_hidden,
		       &attr,
		       &raw.body,
		       raw.major_type, raw.minor_type,
		       raw.a_zzz, raw.b_zzz);

    /* Write it */
    ret = db_put_external5(&ext2, dp, dbip);
    if (ret < 0) bu_log("db5_replace_attributes(%s):  db_put_external5() failure\n",
			dp->d_namep);

    bu_free_external(&attr);
    bu_free_external(&ext2);
    bu_free_external(&ext);		/* 'raw' is now invalid */
    bu_avs_free(avsp);

    return ret;
}


int
db5_update_attributes(struct directory *dp, struct bu_attribute_value_set *avsp, struct db_i *dbip)
{
    struct bu_external ext;
    struct db5_raw_internal raw;
    struct bu_attribute_value_set old_avs;
    struct bu_external attr;
    struct bu_external ext2;
    int ret;

    RT_CK_DIR(dp);
    BU_CK_AVS(avsp);
    RT_CK_DBI(dbip);

    if (RT_G_DEBUG&RT_DEBUG_DB) {
	bu_log("db5_update_attributes(%s) dbip=%p\n",
	       dp->d_namep, (void *)dbip);
	bu_avs_print(avsp, "new attributes");
    }

    if (dbip->dbi_read_only) {
	bu_log("db5_update_attributes(%s):  READ-ONLY file\n",
	       dbip->dbi_filename);
	return -1;
    }

    BU_ASSERT(dbip->dbi_version == 5);

    if (db_get_external(&ext, dp, dbip) < 0)
	return -2;		/* FAIL */

    if (db5_get_raw_internal_ptr(&raw, ext.ext_buf) == NULL) {
	bu_log("db5_update_attributes(%s):  import failure\n",
	       dp->d_namep);
	bu_free_external(&ext);
	return -3;
    }

    /* db5_import_attributes will allocate space */
    bu_avs_init_empty(&old_avs);

    if (raw.attributes.ext_buf) {
	if (db5_import_attributes(&old_avs, &raw.attributes) < 0) {
	    bu_log("db5_update_attributes(%s):  mal-formed attributes in database\n",
		   dp->d_namep);
	    bu_avs_free(&old_avs);
	    bu_free_external(&ext);
	    return -8;
	}
    }

    /* set the material_id field using the material given by the
     * material_name field
     */
    const char *material_name = bu_avs_get(avsp, "material_name");
    if (material_name != NULL && !BU_STR_EQUAL(material_name, "(null)") && !BU_STR_EQUAL(material_name, "del")) {
	struct directory *material_dp = db_lookup(dbip, material_name, LOOKUP_QUIET);
	if (material_dp != RT_DIR_NULL) {
	    struct rt_db_internal intern;
	    struct rt_material_internal *material_ip;
	    if (rt_db_get_internal(&intern, material_dp, dbip, NULL, NULL) >= 0) {
		if (intern.idb_minor_type == DB5_MINORTYPE_BRLCAD_MATERIAL) {
		    material_ip = (struct rt_material_internal *) intern.idb_ptr;
		    const char *id_string = bu_avs_get(&material_ip->physicalProperties, "id");
		    // the material_id will only be set if the material object has an id field set
		    if (id_string != NULL) {
			bu_avs_add(avsp, "material_id", id_string);
		    }
		}
	    }
	}
    }

    bu_avs_merge(&old_avs, avsp);

    db5_export_attributes(&attr, &old_avs);
    BU_EXTERNAL_INIT(&ext2);
    db5_export_object3(&ext2,
		       raw.h_dli,
		       dp->d_namep,
		       raw.h_name_hidden,
		       &attr,
		       &raw.body,
		       raw.major_type, raw.minor_type,
		       raw.a_zzz, raw.b_zzz);

    /* Write it */
    ret = db_put_external5(&ext2, dp, dbip);
    if (ret < 0) {
	bu_log("db5_update_attributes(%s):  db_put_external5() failure\n", dp->d_namep);
    }

    bu_free_external(&attr);
    bu_free_external(&ext2);
    bu_free_external(&ext); /* 'raw' is now invalid */
    bu_avs_free(&old_avs);
    bu_avs_free(avsp);

    return ret;
}


int
db5_update_attribute(const char *obj_name, const char *name, const char *value, struct db_i *dbip)
{
    struct directory *dp;
    struct bu_attribute_value_set avs;

    RT_CK_DBI(dbip);
    if ((dp = db_lookup(dbip, obj_name, LOOKUP_NOISY)) == RT_DIR_NULL)
	return -1;

    bu_avs_init(&avs, 2, "db5_update_attribute");
    bu_avs_add(&avs, name, value);

    return db5_update_attributes(dp, &avs, dbip);
}


int db5_update_ident(struct db_i *dbip, const char *title, double local2mm)
{
    struct bu_attribute_value_set avs;
    struct directory *dp;
    struct bu_vls units = BU_VLS_INIT_ZERO;
    int ret;
    char *old_title = NULL;

    RT_CK_DBI(dbip);

    if ((dp = db_lookup(dbip, DB5_GLOBAL_OBJECT_NAME, LOOKUP_QUIET)) == RT_DIR_NULL) {
	struct bu_external global;
	unsigned char minor_type=0;

	bu_log("db5_update_ident() WARNING: %s object is missing, creating new one.\nYou may have lost important global state when you deleted this object.\n",
	       DB5_GLOBAL_OBJECT_NAME);

	/* OK, make one. Will be empty to start, updated below. */
	db5_export_object3(&global,
			   DB5HDR_HFLAGS_DLI_APPLICATION_DATA_OBJECT,
			   DB5_GLOBAL_OBJECT_NAME, DB5HDR_HFLAGS_HIDDEN_OBJECT, NULL, NULL,
			   DB5_MAJORTYPE_ATTRIBUTE_ONLY, 0,
			   DB5_ZZZ_UNCOMPRESSED, DB5_ZZZ_UNCOMPRESSED);

	dp = db_diradd(dbip, DB5_GLOBAL_OBJECT_NAME, RT_DIR_PHONY_ADDR, 0, 0, (void *)&minor_type);
	dp->d_major_type = DB5_MAJORTYPE_ATTRIBUTE_ONLY;
	if (db_put_external(&global, dp, dbip) < 0) {
	    bu_log("db5_update_ident() unable to create replacement %s object!\n", DB5_GLOBAL_OBJECT_NAME);
	    bu_free_external(&global);
	    return -1;
	}
	bu_free_external(&global);
    }

    bu_vls_printf(&units, "%.25e", local2mm);

    bu_avs_init(&avs, 4, "db5_update_ident");
    bu_avs_add(&avs, "title", title);
    bu_avs_add(&avs, "units", bu_vls_addr(&units));

    ret = db5_update_attributes(dp, &avs, dbip);
    bu_vls_free(&units);
    bu_avs_free(&avs);

    /* protect from losing memory & freeing what we're about to dup */
    if (dbip->dbi_title) {
	old_title = dbip->dbi_title;
    }
    dbip->dbi_title = bu_strdup(title);
    if (old_title) {
	bu_free(old_title, "replaced dbi_title with new");
    }

    return ret;
}


int
db5_fwrite_ident(FILE *fp, const char *title, double local2mm)
{
    struct bu_attribute_value_set avs;
    struct bu_vls units = BU_VLS_INIT_ZERO;
    struct bu_external out;
    struct bu_external attr;
    int result;

    if (local2mm <= 0) {
	bu_log("db5_fwrite_ident(%s, %g) local2mm <= 0\n",
	       title, local2mm);
	return -1;
    }

    /* First, write the header object */
    db5_export_object3(&out, DB5HDR_HFLAGS_DLI_HEADER_OBJECT,
		       NULL, 0, NULL, NULL,
		       DB5_MAJORTYPE_RESERVED, 0,
		       DB5_ZZZ_UNCOMPRESSED, DB5_ZZZ_UNCOMPRESSED);

    result = bu_fwrite_external(fp, &out);
    bu_free_external(&out);
    if (result < 0) {
	return -1;
    }

    /* Second, create the attribute-only object */
    bu_vls_printf(&units, "%.25e", local2mm);

    bu_avs_init(&avs, 4, "db5_fwrite_ident");
    bu_avs_add(&avs, "title", title);
    bu_avs_add(&avs, "units", bu_vls_addr(&units));

    db5_export_attributes(&attr, &avs);
    db5_export_object3(&out, DB5HDR_HFLAGS_DLI_APPLICATION_DATA_OBJECT,
		       DB5_GLOBAL_OBJECT_NAME, DB5HDR_HFLAGS_HIDDEN_OBJECT, &attr, NULL,
		       DB5_MAJORTYPE_ATTRIBUTE_ONLY, 0,
		       DB5_ZZZ_UNCOMPRESSED, DB5_ZZZ_UNCOMPRESSED);

    result = bu_fwrite_external(fp, &out);
    bu_free_external(&out);
    bu_free_external(&attr);
    bu_avs_free(&avs);
    bu_vls_free(&units);
    if (result < 0) {
	return -1;
    }

    return 0;
}


#if defined(USE_BINARY_ATTRIBUTES)
void
decode_binary_attribute(const size_t len, const unsigned char *cp)
{
}
#endif

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
