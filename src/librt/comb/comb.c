/*                          C O M B . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2025 United States Government as represented by
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
/** @addtogroup db5 */
/** @{ */
/** @file comb.c
 *
 * Implement support for combinations in v5 format.
 *
 * The on-disk record looks like this:
 *	width byte
 *	n matrices (only non-identity matrices stored).
 *	n leaves
 *	len of RPN expression.  (len=0 signals all-union expression)
 *	depth of stack
 *	Section 1:  matrices
 *	Section 2:  leaves
 *	Section 3:  (Optional) RPN expression.
 *
 * Encoding of a matrix is (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE)
 * bytes, in network order (big-Endian, IEEE double precision).
 *
 */
/** @} */

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bio.h"

#include "bu/parse.h"
#include "bu/cv.h"
#include "vmath.h"
#include "bn.h"
#include "rt/db5.h"
#include "raytrace.h"

/**
 * Number of bytes used for each value of DB5HDR_WIDTHCODE_*
 */
#define ENCODE_LEN(len) (1 << len)


struct db_tree_counter_state {
    uint32_t magic;
    size_t n_mat;		/* # leaves with non-identity matrices */
    size_t n_leaf;		/* # leaf nodes */
    size_t n_oper;		/* # operator nodes */
    size_t leafbytes;		/* # bytes for name section */
    int non_union_seen;		/* boolean, 1 = non-unions seen */
};
#define DB_TREE_COUNTER_STATE_MAGIC 0x64546373	/* dTcs */
#define DB_CK_TREE_COUNTER_STATE(_p) BU_CKMAG(_p, DB_TREE_COUNTER_STATE_MAGIC, "db_tree_counter_state");


/**
 * Count number of non-identity matrices, number of leaf nodes,
 * number of operator nodes, etc.
 *
 * Returns - maximum depth of stack needed to unpack this tree, if
 * serialized.
 *
 * Notes - We over-estimate the size of the width fields used for
 * holding the matrix subscripts.  The caller is responsible for
 * correcting by saying:
 *
 * tcsp->leafbytes -= tcsp->n_leaf * (8 - ENCODE_LEN(wid));
 */
size_t
db_tree_counter(const union tree *tp, struct db_tree_counter_state *tcsp)
{
    size_t ldepth, rdepth;

    RT_CK_TREE(tp);
    DB_CK_TREE_COUNTER_STATE(tcsp);

    switch (tp->tr_op) {
	case OP_DB_LEAF:
	    tcsp->n_leaf++;
	    if (tp->tr_l.tl_mat && !bn_mat_is_identity(tp->tr_l.tl_mat)) tcsp->n_mat++;
	    /* Over-estimate storage requirement for matrix # */
	    tcsp->leafbytes += strlen(tp->tr_l.tl_name) + 1 + 8;
	    return 1;

	case OP_NOT:
	    /* Unary ops */
	    tcsp->n_oper++;
	    tcsp->non_union_seen = 1;
	    return 1 + db_tree_counter(tp->tr_b.tb_left, tcsp);

	case OP_UNION:
	    /* This node is known to be a binary op */
	    tcsp->n_oper++;
	    ldepth = db_tree_counter(tp->tr_b.tb_left, tcsp);
	    rdepth = db_tree_counter(tp->tr_b.tb_right, tcsp);
	    if (ldepth > rdepth) return ldepth;
	    return rdepth;

	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
	    /* This node is known to be a binary op */
	    tcsp->n_oper++;
	    tcsp->non_union_seen = 1;
	    ldepth = db_tree_counter(tp->tr_b.tb_left, tcsp);
	    rdepth = db_tree_counter(tp->tr_b.tb_right, tcsp);
	    if (ldepth > rdepth) return ldepth;
	    return rdepth;

	default:
	    bu_log("db_tree_counter: bad op %d\n", tp->tr_op);
	    bu_bomb("db_tree_counter\n");
	    /* NOTREACHED */
    }
    /* NOTREACHED */
    return 0;
}


#define DB5COMB_TOKEN_LEAF		1
#define DB5COMB_TOKEN_UNION		2
#define DB5COMB_TOKEN_INTERSECT		3
#define DB5COMB_TOKEN_SUBTRACT		4
#define DB5COMB_TOKEN_XOR		5
#define DB5COMB_TOKEN_NOT		6

struct rt_comb_v5_serialize_state {
    uint32_t magic;
    size_t mat_num;	/* current matrix number */
    size_t nmat;	/* # matrices, total */
    unsigned char *matp;
    unsigned char *leafp;
    unsigned char *exprp;
    int wid;
};
#define RT_COMB_V5_SERIALIZE_STATE_MAGIC 0x43357373	/* C5ss */
#define RT_CK_COMB_V5_SERIALIZE_STATE(_p) BU_CKMAG(_p, RT_COMB_V5_SERIALIZE_STATE_MAGIC, "rt_comb_v5_serialize_state")


/**
 * In one single pass through the tree, serialize out all three output
 * sections at once.
 */
void
rt_comb_v5_serialize(
    union tree *tp,
    struct rt_comb_v5_serialize_state *ssp)
{
    size_t n;
    ssize_t mi;

    RT_CK_TREE(tp);
    RT_CK_COMB_V5_SERIALIZE_STATE(ssp);

    /* setup stack for iterative traversal */
    union tree** stack = (union tree**)0;
    union tree** rev = (union tree**)0;
    union tree* curr = NULL;
    int size = 128;
    int idx = 0;
    int depth = 0;
    stack = (union tree**)bu_malloc(sizeof(union tree*) * size, "init stack");
    rev = (union tree**)bu_malloc(sizeof(union tree*) * size, "init rev");
    stack[idx++] = TREE_NULL;
    stack[idx++] = tp;
    rev[depth] = TREE_NULL;

    /* the first traversal gives us a reversed order */
    while ((curr = stack[--idx]) != TREE_NULL) {
        if (depth++ >= size - 1 || idx >= size - 3) {
            size <<= 1;
            stack = (union tree**)bu_realloc(stack, sizeof(union tree*) * size, "double stack");
            rev = (union tree**)bu_realloc(rev, sizeof(union tree*) * size, "double rev");
        }
        rev[depth] = curr;

        if (curr->tr_b.tb_left && curr->tr_op != OP_DB_LEAF)
            stack[idx++] = curr->tr_b.tb_left;

        if (curr->tr_b.tb_right && curr->tr_op != OP_DB_LEAF)
            stack[idx++] = curr->tr_b.tb_right;
    }
    bu_free(stack, "free stack");
    depth++;

    /* actually do the serialize with the correct order */
    while ((curr = rev[--depth]) != TREE_NULL) {
        switch (curr->tr_op) {
    	    case OP_DB_LEAF:
    	        /*
    	        * Encoding of the leaf: A null-terminated name string,
    	        * and the matrix subscript.  -1 == identity.
    	        */
    	        n = strlen(curr->tr_l.tl_name) + 1;
    	        memcpy(ssp->leafp, curr->tr_l.tl_name, n);
    	        ssp->leafp += n;
    
    	        if (curr->tr_l.tl_mat && !bn_mat_is_identity(curr->tr_l.tl_mat)) {
    		    mi = ssp->mat_num++;
    	        } else {
    		    mi = (ssize_t)-1;
    	        }
    
    	        BU_ASSERT(mi < (ssize_t)ssp->nmat);
    
    	        /* there should be a better way than casting
    	        * 'mi' from ssize_t to size_t
    	        */
    	        n = (size_t)mi;
    	        ssp->leafp = db5_encode_length(ssp->leafp, n, ssp->wid);
    
    	        /* Encoding of the matrix */
    	        if (mi != (ssize_t)-1) {
    		    /* must be double for import and export */
    		    double scanmat[ELEMENTS_PER_MAT];
    
    		    MAT_COPY(scanmat, curr->tr_l.tl_mat); /* convert fastf_t to double */
    		    bu_cv_htond(ssp->matp, (const unsigned char *)scanmat, ELEMENTS_PER_MAT);
    		    ssp->matp += ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE;
    	        }
    
    	        /* Encoding of the "leaf" operator */
    	        if (ssp->exprp)
    		    *ssp->exprp++ = DB5COMB_TOKEN_LEAF;
    	        break;
    
    	    case OP_NOT:
    	        /* Unary ops */
    	        if (ssp->exprp)
    		    *ssp->exprp++ = DB5COMB_TOKEN_NOT;
    	        break;
    
    	    case OP_UNION:
    	        /* This node is known to be a binary op */
    	        if (ssp->exprp)
    		    *ssp->exprp++ = DB5COMB_TOKEN_UNION;
    	        break;
    	    case OP_INTERSECT:
    	        /* This node is known to be a binary op */
    	        if (ssp->exprp)
    		    *ssp->exprp++ = DB5COMB_TOKEN_INTERSECT;
    	        break;
    	    case OP_SUBTRACT:
    	        /* This node is known to be a binary op */
    	        if (ssp->exprp)
    		    *ssp->exprp++ = DB5COMB_TOKEN_SUBTRACT;
    	        break;
    	    case OP_XOR:
    	        /* This node is known to be a binary op */
    	        if (ssp->exprp)
    		    *ssp->exprp++ = DB5COMB_TOKEN_XOR;
    	        break;
    
    	    default:
    	        bu_log("rt_comb_v5_serialize: bad op %d\n", curr->tr_op);
    	        bu_bomb("rt_comb_v5_serialize\n");
        }
    }
    bu_free(rev, "free rev");
}


int
rt_comb_export5(
    struct bu_external *ep,
    const struct rt_db_internal *ip,
    double UNUSED(local2mm),
    const struct db_i *dbip,
    struct resource *resp)
{
    struct rt_comb_internal *comb;
    struct db_tree_counter_state tcs;
    struct rt_comb_v5_serialize_state ss;
    long max_stack_depth;
    size_t need;
    size_t rpn_len = 0;	/* # items in RPN expression */
    int wid; /* encode format */
    unsigned char *cp;
    unsigned char *leafp_end;
    struct bu_attribute_value_set *avsp;
    struct bu_vls value = BU_VLS_INIT_ZERO;

    /* check inputs */
    RT_CK_DB_INTERNAL(ip);
    if (dbip) RT_CK_DBI(dbip);
    if (resp) RT_CK_RESOURCE(resp);

    /* validate it's a comb */
    if (ip->idb_type != ID_COMBINATION) bu_bomb("rt_comb_export5() type not ID_COMBINATION");
    comb = (struct rt_comb_internal *)ip->idb_ptr;
    RT_CK_COMB(comb);

    /* First pass -- count number of non-identity matrices, number of
     * leaf nodes, number of operator nodes.
     */
    memset((char *)&tcs, 0, sizeof(tcs));
    tcs.magic = DB_TREE_COUNTER_STATE_MAGIC;
    if (comb->tree)
	max_stack_depth = db_tree_counter(comb->tree, &tcs);
    else
	max_stack_depth = 0;	/* some combinations have no tree */

    if (tcs.non_union_seen) {
	/* RPN expression needs one byte for each leaf or operator node */
	rpn_len = tcs.n_leaf + tcs.n_oper;
    } else {
	rpn_len = 0;
    }

    wid = db5_select_length_encoding(
	tcs.n_mat | tcs.n_leaf | tcs.leafbytes |
	rpn_len | max_stack_depth);

    /* Apply correction factor to tcs.leafbytes now that we know
     * 'wid'.  Ignore the slight chance that a smaller 'wid' might now
     * be possible.
     */
    tcs.leafbytes -= tcs.n_leaf * (8 - ENCODE_LEN(wid));

    /* Second pass -- determine amount of on-disk storage needed */
    need =  1 +			/* width code */
	ENCODE_LEN(wid) + 	/* size for nmatrices */
	ENCODE_LEN(wid) +	/* size for nleaves */
	ENCODE_LEN(wid) +	/* size for leafbytes */
	ENCODE_LEN(wid) +	/* size for len of RPN */
	ENCODE_LEN(wid) +	/* size for max_stack_depth */
	tcs.n_mat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE) +	/* sizeof matrix array */
	tcs.leafbytes +		/* size for leaf nodes */
	rpn_len;		/* storage for RPN expression */

    BU_EXTERNAL_INIT(ep);
    ep->ext_nbytes = need;
    ep->ext_buf = (uint8_t *)bu_calloc(1, need, "rt_comb_export5 ext_buf");

    /* Build combination's on-disk header section */
    cp = (unsigned char *)ep->ext_buf;
    *cp++ = wid;
    cp = db5_encode_length(cp, tcs.n_mat, wid);
    cp = db5_encode_length(cp, tcs.n_leaf, wid);
    cp = db5_encode_length(cp, tcs.leafbytes, wid);
    cp = db5_encode_length(cp, rpn_len, wid);
    cp = db5_encode_length(cp, max_stack_depth, wid);

    /*
     * The output format has three sections:
     * Section 1:  matrices
     * Section 2:  leaf nodes
     * Section 3:  Optional RPN expression
     *
     * We have pre-computed the exact size of all three sections, so
     * they can all be serialized together in one pass.  Establish
     * pointers to the start of each section.
     */
    ss.magic = RT_COMB_V5_SERIALIZE_STATE_MAGIC;
    ss.wid = wid;
    ss.mat_num = 0;
    ss.nmat = tcs.n_mat;
    ss.matp = cp;
    ss.leafp = cp + tcs.n_mat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE);
    leafp_end = ss.leafp + tcs.leafbytes;
    if (rpn_len)
	ss.exprp = leafp_end;
    else
	ss.exprp = NULL;

    if (comb->tree)
	rt_comb_v5_serialize(comb->tree, &ss);

    BU_ASSERT(ss.mat_num == tcs.n_mat);
    BU_ASSERT(ss.matp == cp + tcs.n_mat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE));
    BU_ASSERT(ss.leafp == leafp_end);
    if (rpn_len)
	BU_ASSERT(ss.exprp <= ((unsigned char *)ep->ext_buf) + ep->ext_nbytes);

    /* Encode all the other stuff as attributes. */
    /* WARNING:  We remove const from the ip pointer!!! */
    avsp = (struct bu_attribute_value_set *)&ip->idb_avs;
    if (avsp->magic != BU_AVS_MAGIC)
	bu_avs_init(avsp, 32, "rt_comb v5 attributes");
    if (comb->region_flag) {
	/* Presence of this attribute means this comb is a region.
	 * Current code values are 0, 1, and 2; all are regions.  See
	 * raytrace.h for meanings of different values
	 */
	bu_vls_trunc(&value, 0);
	switch (comb->is_fastgen) {
	    case REGION_FASTGEN_PLATE:
		bu_vls_printf(&value, "P");
		break;
	    case REGION_FASTGEN_VOLUME:
		bu_vls_printf(&value, "V");
		break;
	    case REGION_NON_FASTGEN: /* fallthrough */
	    default:
		bu_vls_printf(&value, "R");
		break;
	}
	bu_avs_add_vls(avsp, "region", &value);
    } else
	bu_avs_remove(avsp, "region");

    if (comb->inherit)
	bu_avs_add(avsp, "inherit", "1");
    else
	bu_avs_remove(avsp, "inherit");

    if (comb->rgb_valid) {
	bu_vls_trunc(&value, 0);
	bu_vls_printf(&value, "%d/%d/%d", V3ARGS(comb->rgb));
	bu_avs_add_vls(avsp, "rgb", &value);
    } else
	bu_avs_remove(avsp, "rgb");

    /* optical shader string goes in an attribute */
    if (bu_vls_strlen(&comb->shader) > 0) {
	/* NOTE: still writing out an 'oshader' attribute in addition
	 * to a 'shader' attribute so that older versions of BRL-CAD
	 * will find the shader information correctly.
	 */
	bu_avs_add_vls(avsp, "oshader", &comb->shader);
	bu_avs_add_vls(avsp, "shader", &comb->shader);
    } else {
	/* see NOTE above */
	bu_avs_remove(avsp, "oshader");
	bu_avs_remove(avsp, "shader");
    }

    /* GIFT compatibility */
    if (comb->region_id != 0) {
	bu_vls_trunc(&value, 0);
	bu_vls_printf(&value, "%ld", comb->region_id);
	bu_avs_add_vls(avsp, "region_id", &value);
    } else
	bu_avs_remove(avsp, "region_id");

    if (comb->aircode != 0) {
	bu_vls_trunc(&value, 0);
	bu_vls_printf(&value, "%ld", comb->aircode);
	bu_avs_add_vls(avsp, "aircode", &value);
    } else
	bu_avs_remove(avsp, "aircode");

    if (comb->GIFTmater != 0) {
	bu_vls_trunc(&value, 0);
	bu_vls_printf(&value, "%ld", comb->GIFTmater);
	bu_avs_add_vls(avsp, "material_id", &value);
    } else
	bu_avs_remove(avsp, "material_id");

    if (comb->los != 0) {
	bu_vls_trunc(&value, 0);
	bu_vls_printf(&value, "%ld", comb->los);
	bu_avs_add_vls(avsp, "los", &value);
    } else
	bu_avs_remove(avsp, "los");

    bu_vls_free(&value);
    return 0;	/* OK */
}

/* We assume the calling function has already verified that
 * mat is not an identity matrix, to avoid having to do the
 * check over and over while walking. */
static void
_comb_mat_leaf(const mat_t mat, union tree *tp)
{
    if (!tp)
	return;
    RT_CK_TREE(tp);

    switch (tp->tr_op) {
	        case OP_UNION:
        case OP_INTERSECT:
        case OP_SUBTRACT:
        case OP_XOR:
            _comb_mat_leaf(mat, tp->tr_b.tb_right);
            /* fall through */
        case OP_NOT:
        case OP_GUARD:
        case OP_XNOP:
            _comb_mat_leaf(mat, tp->tr_b.tb_left);
            break;
        case OP_DB_LEAF:
	    if (tp->tr_l.tl_mat) {
		mat_t mat_tmp;
		MAT_COPY(mat_tmp, tp->tr_l.tl_mat);
		bn_mat_mul(tp->tr_l.tl_mat, mat, mat_tmp);
		/* Check if we created an identity matrix */
		if (bn_mat_is_identity(tp->tr_l.tl_mat)) {
		    bu_free((char *)tp->tr_l.tl_mat, "tl_mat");
		    tp->tr_l.tl_mat = NULL;
		}
	    } else {
		tp->tr_l.tl_mat = bn_mat_dup(mat);
	    }
	    break;
	default:
            bu_log("_comb_mat_leaf: unrecognized operator %d\n", tp->tr_op);
            bu_bomb("_comb_mat_leaf\n");
    }
}

int
rt_comb_mat(struct rt_db_internal *rop, const mat_t mat, const struct rt_db_internal *ip)
{
    if (!rop || !mat)
	return BRLCAD_OK;

    // Identity matrix is a no-op
    if (bn_mat_is_identity(mat))
	return BRLCAD_OK;

    // For the moment, we only support applying a mat to combs in place - the
    // input and output must be the same.
    if (ip && rop != ip) {
	bu_log("rt_comb_mat:  verification of matching comb trees is unsupported - input comb must be the same as the output comb.\n");
	return BRLCAD_ERROR;
    }

    struct rt_comb_internal *comb = (struct rt_comb_internal *)rop->idb_ptr;
    RT_CK_COMB(comb);
    _comb_mat_leaf(mat, comb->tree);

    return BRLCAD_OK;
}

int
rt_comb_import5(struct rt_db_internal *ip, const struct bu_external *ep,
		const mat_t mat, const struct db_i *dbip, struct resource *resp)
{
    struct rt_comb_internal *comb = NULL;
    unsigned char *cp = NULL;
    int wid = 0;
    size_t nmat = 0;
    size_t nleaf = 0;
    size_t rpn_len = 0;
    size_t max_stack_depth = 0;
    size_t leafbytes = 0;
    unsigned char *matp = NULL;
    unsigned char *leafp = NULL;
    unsigned char *leafp_end = NULL;
    unsigned char *exprp = NULL;
#define MAX_V5_STACK 8000
    union tree *stack[MAX_V5_STACK] = {NULL};
    union tree **sp = NULL;			/* stack pointer */
    const char *ap = NULL;
    size_t ius = 0;

    RT_CK_DB_INTERNAL(ip);
    BU_CK_EXTERNAL(ep);
    RT_CK_DBI(dbip);
    RT_CK_RESOURCE(resp);

    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_COMBINATION;
    ip->idb_meth = &OBJ[ID_COMBINATION];

    BU_ALLOC(comb, struct rt_comb_internal);
    RT_COMB_INTERNAL_INIT(comb);

    ip->idb_ptr = (void *)comb;

    cp = ep->ext_buf;
    wid = *cp++;
    cp += db5_decode_length(&nmat, cp, wid);
    cp += db5_decode_length(&nleaf, cp, wid);
    cp += db5_decode_length(&leafbytes, cp, wid);
    cp += db5_decode_length(&rpn_len, cp, wid);
    cp += db5_decode_length(&max_stack_depth, cp, wid);
    matp = cp;
    leafp = cp + nmat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE);
    exprp = leafp + leafbytes;
    leafp_end = exprp;

    if (rpn_len == 0) {
	ssize_t is;

	/* This tree is all union operators, import it as a balanced tree */
	struct bu_ptbl *tbl1, *tbl2;

	BU_ALLOC(tbl1, struct bu_ptbl);
	BU_ALLOC(tbl2, struct bu_ptbl);

	/* insert all the leaf nodes into a bu_ptbl */
	bu_ptbl_init(tbl1, nleaf, "rt_comb_import5: tbl");
	for (is = nleaf-1; is >= 0; is--) {
	    union tree *tp;
	    size_t mi;

	    BU_GET(tp, union tree);
	    RT_TREE_INIT(tp);
	    tp->tr_l.tl_op = OP_DB_LEAF;
	    tp->tr_l.tl_name = bu_strdup((const char *)leafp);
	    leafp += strlen((const char *)leafp) + 1;

	    /* Get matrix index */
	    mi = 4095;			/* sanity */
	    leafp += db5_decode_signed(&mi, leafp, wid);

	    if (mi == (size_t)-1) {
		/* Signal identity matrix */
		tp->tr_l.tl_mat = (matp_t)NULL;
	    } else {
		mat_t diskmat;

		/* must be double for import and export */
		double scanmat[16];

		/* Unpack indicated matrix mi */
		BU_ASSERT(mi < nmat);

		/* read matrix */
		bu_cv_ntohd((unsigned char *)scanmat, &matp[mi*ELEMENTS_PER_MAT*SIZEOF_NETWORK_DOUBLE], ELEMENTS_PER_MAT);
		MAT_COPY(diskmat, scanmat); /* convert double to fastf_t */

		tp->tr_l.tl_mat = bn_mat_dup(diskmat);
	    }
	    bu_ptbl_ins(tbl1, (long *)tp);
	}

	/* use a second bu_ptbl to help build a balanced tree
	 * 1 - pick off pairs of pointers from tbl1
	 * 2 - make a small tree that unions the pair
	 * 3 - insert that tree into tbl2
	 * 4 - insert any leftover pointer from tbl1 into tbl2
	 * 5 - swap tbl1 and tbl2
	 * 6 - truncate tbl2 and go to step 1
	 * stop when tbl2 has less than 2 members
	 */
	bu_ptbl_init(tbl2, (BU_PTBL_LEN(tbl1) + 1)/2, "rt_comb_import5: tbl1");
	while (1) {
	    struct bu_ptbl *tmp;

	    for (ius = 0; ius < BU_PTBL_LEN(tbl1); ius += 2) {
		union tree *tp1, *tp2, *unionp;
		size_t j;

		j = ius + 1;
		tp1 = (union tree *)BU_PTBL_GET(tbl1, ius);
		if (j < BU_PTBL_LEN(tbl1)) {
		    tp2 = (union tree *)BU_PTBL_GET(tbl1, j);
		} else {
		    tp2 = (union tree *)NULL;
		}

		if (tp2) {
		    BU_GET(unionp, union tree);
		    RT_TREE_INIT(unionp);
		    unionp->tr_b.tb_op = OP_UNION;
		    unionp->tr_b.tb_left = tp1;
		    unionp->tr_b.tb_right = tp2;
		    bu_ptbl_ins(tbl2, (long *)unionp);
		} else {
		    bu_ptbl_ins(tbl2, (long *)tp1);
		}

	    }

	    if (BU_PTBL_LEN(tbl2) == 0) {
		comb->tree = (union tree *)NULL;
		bu_ptbl_free(tbl1);
		bu_ptbl_free(tbl2);
		bu_free((char *)tbl1, "rt_comb_import5: tbl1");
		bu_free((char *)tbl2, "rt_comb_import5: tbl2");
		break;
	    } else if (BU_PTBL_LEN(tbl2) == 1) {
		comb->tree = (union tree *)BU_PTBL_GET(tbl2, 0);
		bu_ptbl_free(tbl1);
		bu_ptbl_free(tbl2);
		bu_free((char *)tbl1, "rt_comb_import5: tbl1");
		bu_free((char *)tbl2, "rt_comb_import5: tbl2");
		break;
	    }

	    tmp = tbl2;
	    tbl2 = tbl1;
	    tbl1 = tmp;
	    bu_ptbl_trunc(tbl2, 0);
	}
	BU_ASSERT(leafp == leafp_end);
	goto finish;
    }

    /*
     * Bring the RPN expression back from the disk, populating leaves
     * and matrices in the order they are encountered.
     */
    if (max_stack_depth > MAX_V5_STACK) {
	bu_log("Combination needs stack depth %zu, only have %d, aborted\n",
	       max_stack_depth, MAX_V5_STACK);
	return -1;
    }
    sp = &stack[0];

    for (ius = 0; ius < rpn_len; ius++, exprp++) {
	union tree *tp;
	size_t mi;

	BU_GET(tp, union tree);
	RT_TREE_INIT(tp);

	switch (*exprp) {
	    case DB5COMB_TOKEN_LEAF:
		tp->tr_l.tl_op = OP_DB_LEAF;
		tp->tr_l.tl_name = bu_strdup((const char *)leafp);
		leafp += strlen((const char *)leafp) + 1;

		/* Get matrix index */
		mi = 4095;			/* sanity */
		leafp += db5_decode_signed(&mi, leafp, wid);

		if ((ssize_t)mi < 0) {
		    /* Signal identity matrix */
		    tp->tr_l.tl_mat = (matp_t)NULL;
		} else {
		    mat_t diskmat;

		    /* must be double for import and export */
		    double scanmat[16];

		    /* Unpack indicated matrix mi */
		    BU_ASSERT(mi < nmat);

		    /* read matrix */
		    bu_cv_ntohd((unsigned char *)scanmat, &matp[mi*ELEMENTS_PER_MAT*SIZEOF_NETWORK_DOUBLE], ELEMENTS_PER_MAT);
		    MAT_COPY(diskmat, scanmat); /* convert double to fastf_t */

		    tp->tr_l.tl_mat = bn_mat_dup(diskmat);
		}
		break;

	    case DB5COMB_TOKEN_UNION:
	    case DB5COMB_TOKEN_INTERSECT:
	    case DB5COMB_TOKEN_SUBTRACT:
	    case DB5COMB_TOKEN_XOR:
		/* These are all binary operators */
		tp->tr_b.tb_regionp = REGION_NULL;
		tp->tr_b.tb_right = *--sp;
		RT_CK_TREE(tp->tr_b.tb_right);
		tp->tr_b.tb_left = *--sp;
		RT_CK_TREE(tp->tr_b.tb_left);
		switch (*exprp) {
		    case DB5COMB_TOKEN_UNION:
			tp->tr_b.tb_op = OP_UNION;
			break;
		    case DB5COMB_TOKEN_INTERSECT:
			tp->tr_b.tb_op = OP_INTERSECT;
			break;
		    case DB5COMB_TOKEN_SUBTRACT:
			tp->tr_b.tb_op = OP_SUBTRACT;
			break;
		    case DB5COMB_TOKEN_XOR:
			tp->tr_b.tb_op = OP_XOR;
			break;
		}
		break;

	    case DB5COMB_TOKEN_NOT:
		/* This is a unary operator */
		tp->tr_b.tb_regionp = REGION_NULL;
		tp->tr_b.tb_left = *--sp;
		RT_CK_TREE(tp->tr_b.tb_left);
		tp->tr_b.tb_right = TREE_NULL;
		tp->tr_b.tb_op = OP_NOT;
		break;
	    default:
		bu_log("rt_comb_import5() unknown RPN expression token=%d, import aborted\n", *exprp);
		return -1;
	}

	/* Push this node on the stack */
	*sp++ = tp;
    }
    BU_ASSERT(leafp == leafp_end);

    /* There should only be one thing left on the stack, the result */
    BU_ASSERT(sp == &stack[1]);

    comb->tree = stack[0];
    RT_CK_TREE(comb->tree);

finish:

    /* Apply transform */
    rt_comb_mat(ip, mat, NULL);

    if (ip->idb_avs.magic != BU_AVS_MAGIC) return 0;	/* OK */

    /* Unpack the attributes */
    comb->rgb_valid = 0;

    if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_COLOR))) != NULL) {
	int ibuf[3];
	if (sscanf(ap, "%d/%d/%d", ibuf, ibuf+1, ibuf+2) == 3) {
	    VMOVE(comb->rgb, ibuf);
	    comb->rgb_valid = 1;
	} else {
	    bu_log("unable to parse 'rgb' attribute '%s'\n", ap);
	}
    }
    if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_INHERIT))) != NULL) {
	comb->inherit = atoi(ap);
    }
    if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_REGION))) != NULL) {
	/* Presence of this attribute implies it is a region */
	comb->region_flag = 1;

	/* Determine if this is a FASTGEN region */
	switch (*ap) {
	    case 'V' : /* fallthrough */
	    case '2' :
		comb->is_fastgen = REGION_FASTGEN_VOLUME;
		break;
	    case 'P' : /* fallthrough */
	    case '1' :
		comb->is_fastgen = REGION_FASTGEN_PLATE;
		break;
	    case 'R' : /* fallthrough */
	    case '0' :
		comb->is_fastgen = REGION_NON_FASTGEN;
		break;
	    default:
		bu_log("unable to parse 'region' attribute '%s'\n", ap);
		break;
	}

	/* get the other GIFT "region" attributes */
	if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_REGION_ID))) != NULL) {
	    comb->region_id = atol(ap);
	}
	if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_AIR))) != NULL) {
	    comb->aircode = atol(ap);
	}
	if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_MATERIAL_ID))) != NULL) {
	    comb->GIFTmater = atol(ap);
	}
	if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_MATERIAL_NAME))) != NULL) {
	    bu_vls_strcpy(&comb->material, ap);
	}
	if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_LOS))) != NULL) {
	    comb->los = atol(ap);
	}
    }
    if ((ap = bu_avs_get(&ip->idb_avs, db5_standard_attribute(ATTR_SHADER))) != NULL) {
	bu_vls_strcat(&comb->shader, ap);
    }

    /* Combs need to know about their database context. */
    comb->src_dbip = dbip;

    return 0; /* OK */
}


/**
 * Sets the result string to a description of the given combination.
 * Entered via OBJ[].ft_get().
 */
int
rt_comb_get(struct bu_vls *logstr, const struct rt_db_internal *intern, const char *item)
{
    const struct rt_comb_internal *comb;
    char buf[128];

    RT_CK_DB_INTERNAL(intern);
    comb = (struct rt_comb_internal *)intern->idb_ptr;
    RT_CK_COMB(comb);

    if (item == 0) {
	/* Print out the whole combination. */

	bu_vls_printf(logstr, "comb region ");
	if (comb->region_flag) {
	    bu_vls_printf(logstr, "yes id %ld ", comb->region_id);

	    if (comb->aircode) {
		bu_vls_printf(logstr, "air %ld ", comb->aircode);
	    }
	    if (comb->los) {
		bu_vls_printf(logstr, "los %ld ", comb->los);
	    }

	    if (comb->GIFTmater) {
		bu_vls_printf(logstr, "GIFTmater %ld ", comb->GIFTmater);
	    }
	} else {
	    bu_vls_printf(logstr, "no ");
	}

	if (comb->rgb_valid) {
	    bu_vls_printf(logstr, "rgb {%d %d %d} ", V3ARGS(comb->rgb));
	}

	if (bu_vls_strlen(&comb->shader) > 0) {
	    bu_vls_printf(logstr, "shader {%s} ", bu_vls_addr(&comb->shader));
	}

	if (bu_vls_strlen(&comb->material) > 0) {
	    bu_vls_printf(logstr, "material {%s} ", bu_vls_addr(&comb->material));
	}

	if (comb->inherit) {
	    bu_vls_printf(logstr, "inherit yes ");
	}

	bu_vls_printf(logstr, "tree {");
	db_tree_list(logstr, comb->tree);
	bu_vls_putc(logstr, '}');

	return BRLCAD_OK;
    } else {
	/* Print out only the requested item. */
	register int i;
	char itemlwr[128];

	for (i = 0; i < 127 && item[i]; i++) {
	    itemlwr[i] = (isupper((int)item[i]) ? tolower((int)item[i]) :
			  item[i]);
	}
	itemlwr[i] = '\0';

	if (BU_STR_EQUIV(itemlwr, "region")) {
	    snprintf(buf, 128, "%s", comb->region_flag ? "yes" : "no");
	} else if (BU_STR_EQUIV(itemlwr, "id")) {
	    if (!comb->region_flag) goto not_region;
	    snprintf(buf, 128, "%ld", comb->region_id);
	} else if (BU_STR_EQUIV(itemlwr, "air")) {
	    if (!comb->region_flag) goto not_region;
	    snprintf(buf, 128, "%ld", comb->aircode);
	} else if (BU_STR_EQUIV(itemlwr, "los")) {
	    if (!comb->region_flag) goto not_region;
	    snprintf(buf, 128, "%ld", comb->los);
	} else if (BU_STR_EQUIV(itemlwr, "giftmater")) {
	    if (!comb->region_flag) goto not_region;
	    snprintf(buf, 128, "%ld", comb->GIFTmater);
	} else if (BU_STR_EQUIV(itemlwr, "rgb")) {
	    if (comb->rgb_valid)
		snprintf(buf, 128, "%d %d %d", V3ARGS(comb->rgb));
	    else
		snprintf(buf, 128, "invalid");
	} else if (BU_STR_EQUIV(itemlwr, "shader")) {
	    bu_vls_printf(logstr, "%s", bu_vls_addr(&comb->shader));
	    return BRLCAD_OK;
	} else if (BU_STR_EQUIV(itemlwr, "material")) {
	    bu_vls_printf(logstr, "%s", bu_vls_addr(&comb->material));
	    return BRLCAD_OK;
	} else if (BU_STR_EQUIV(itemlwr, "inherit")) {
	    snprintf(buf, 128, "%s", comb->inherit ? "yes" : "no");
	} else if (BU_STR_EQUIV(itemlwr, "tree")) {
	    db_tree_list(logstr, comb->tree);
	    return BRLCAD_OK;
	} else {
	    bu_vls_printf(logstr, "no such attribute");
	    return BRLCAD_ERROR;
	}

	bu_vls_printf(logstr, "%s", buf);
	return BRLCAD_OK;

    not_region:
	bu_vls_printf(logstr, "item not valid for non-region");
	return BRLCAD_ERROR;
    }
}


/**
 * Example -
 * rgb "1 2 3" ...
 *
 * Invoked via OBJ[ID_COMBINATION].ft_adjust()
 */
int
rt_comb_adjust(struct bu_vls *logstr, struct rt_db_internal *intern, int argc, char **argv)
{
    struct rt_comb_internal *comb;
    char buf[1024] = {'\0'};
    int i;
    double d;

    RT_CK_DB_INTERNAL(intern);
    comb = (struct rt_comb_internal *)intern->idb_ptr;
    RT_CK_COMB(comb);

    while (argc >= 2) {
	/* Force to lower case */
	for (i = 0; i < 1023 && argv[0][i] != '\0'; i++) {
	    buf[i] = tolower((int)argv[0][i]);
	}
	buf[i] = '\0';

	if (BU_STR_EQUIV(buf, "region")) {
	    if (BU_STR_EQUIV(argv[1], "none")) {
		comb->region_flag = 0;
	    } else if (bu_str_false(argv[1])) {
		comb->region_flag = 0;
	    } else if (bu_str_true(argv[1])) {
		comb->region_flag = 1;
	    } else {
		if (sscanf(argv[1], "%d", &i) != 1)
		    return BRLCAD_ERROR;

		if (i != 0)
		    i = 1;

		comb->region_flag = (char)i;
	    }
	} else if (BU_STR_EQUIV(buf, "temp")) {
	    if (!comb->region_flag) goto not_region;
	    if (BU_STR_EQUIV(argv[1], "none")) {
		comb->temperature = 0.0;
	    } else {
		if (sscanf(argv[1], "%lf", &d) != 1)
		    return BRLCAD_ERROR;
		comb->temperature = (float)d;
	    }
	} else if (BU_STR_EQUIV(buf, "id")) {
	    if (!comb->region_flag) goto not_region;
	    if (BU_STR_EQUIV(argv[1], "none")) {
		comb->region_id = 0;
	    } else {
		if (sscanf(argv[1], "%d", &i) != 1)
		    return BRLCAD_ERROR;
		comb->region_id = i;
	    }
	} else if (BU_STR_EQUIV(buf, "air")) {
	    if (!comb->region_flag) goto not_region;
	    if (BU_STR_EQUIV(argv[1], "none")) {
		comb->aircode = 0;
	    } else {
		if (sscanf(argv[1], "%d", &i) != 1)
		    return BRLCAD_ERROR;
		comb->aircode = i;
	    }
	} else if (BU_STR_EQUIV(buf, "los")) {
	    if (!comb->region_flag) goto not_region;
	    if (BU_STR_EQUIV(argv[1], "none")) {
		comb->los = 0;
	    } else {
		if (sscanf(argv[1], "%d", &i) != 1)
		    return BRLCAD_ERROR;
		comb->los = i;
	    }
	} else if (BU_STR_EQUIV(buf, "giftmater")) {
	    if (!comb->region_flag) goto not_region;
	    if (BU_STR_EQUIV(argv[1], "none")) {
		comb->GIFTmater = 0;
	    } else {
		if (sscanf(argv[1], "%d", &i) != 1)
		    return BRLCAD_ERROR;
		comb->GIFTmater = i;
	    }
	} else if (db5_standardize_attribute(buf) == ATTR_COLOR) {
	    if (BU_STR_EQUIV(argv[1], "invalid") || BU_STR_EQUIV(argv[1], "none")) {
		comb->rgb[0] = comb->rgb[1] =
		    comb->rgb[2] = 0;
		comb->rgb_valid = 0;
	    } else {
		unsigned int r, g, b;
		i = sscanf(argv[1], "%u %u %u",
			   &r, &g, &b);
		if (i != 3) {
		    bu_vls_printf(logstr, "adjust %s: not valid rgb 3-tuple\n", argv[1]);
		    return BRLCAD_ERROR;
		}
		comb->rgb[0] = (unsigned char)r;
		comb->rgb[1] = (unsigned char)g;
		comb->rgb[2] = (unsigned char)b;
		comb->rgb_valid = 1;
	    }
	} else if (BU_STR_EQUIV(buf, "shader")) {
	    bu_vls_trunc(&comb->shader, 0);
	    if (!BU_STR_EQUIV(argv[1], "none")) {
		bu_vls_strcat(&comb->shader, argv[1]);
		/* Leading spaces boggle the combination exporter */
		bu_vls_trimspace(&comb->shader);
	    }
	} else if (BU_STR_EQUIV(buf, "material")) {
	    bu_vls_trunc(&comb->material, 0);
	    if (!BU_STR_EQUIV(argv[1], "none")) {
		bu_vls_strcat(&comb->material, argv[1]);
		bu_vls_trimspace(&comb->material);
	    }
	} else if (BU_STR_EQUIV(buf, "inherit")) {
	    if (BU_STR_EQUIV(argv[1], "none")) {
		comb->inherit = 0;
	    } else if (bu_str_false(argv[1])) {
		comb->inherit = 0;
	    } else if (bu_str_true(argv[1])) {
		comb->inherit = 1;
	    } else {
		if (sscanf(argv[1], "%d", &i) != 1)
		    return BRLCAD_ERROR;

		if (i != 0)
		    i = 1;

		comb->inherit = (char)i;
	    }
	} else if (BU_STR_EQUIV(buf, "tree")) {
	    union tree *newtree;

	    if (*argv[1] == '\0' || BU_STR_EQUIV(argv[1], "none")) {
		db_free_tree(comb->tree, &rt_uniresource);
		comb->tree = TREE_NULL;
	    } else {
		newtree = db_tree_parse(logstr, argv[1], &rt_uniresource);
		if (newtree == TREE_NULL) {
		    bu_vls_printf(logstr, "db adjust tree: bad tree '%s'\n", argv[1]);
		    return BRLCAD_ERROR;
		}
		db_free_tree(comb->tree, &rt_uniresource);
		comb->tree = newtree;
	    }
	} else {
	    bu_vls_printf(logstr, "db adjust %s : no such attribute", buf);
	    return BRLCAD_ERROR;
	}
	argc -= 2;
	argv += 2;
    }

    /* Make sure the attributes have gotten the message */
    db5_sync_comb_to_attr(&intern->idb_avs, comb);

    return BRLCAD_OK;

not_region:
    bu_vls_printf(logstr, "adjusting attribute %s is not valid for a non-region combination.", buf);
    return BRLCAD_ERROR;
}


int
rt_comb_form(struct bu_vls *logstr, const struct rt_functab *ftp)
{
    RT_CK_FUNCTAB(ftp);

    bu_vls_printf(logstr, "region {%%s} id {%%d} air {%%d} los {%%d} GIFTmater {%%d} rgb {%%d %%d %%d} shader {%%s} material {%%s} inherit {%%s} tree {%%s}");

    return BRLCAD_OK;
}


/**
 * Create a blank combination with appropriate values.  Called via
 * OBJ[ID_COMBINATION].ft_make().
 */
void
rt_comb_make(const struct rt_functab *UNUSED(ftp), struct rt_db_internal *intern)
{
    struct rt_comb_internal *comb;

    intern->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    intern->idb_type = ID_COMBINATION;
    intern->idb_meth = &OBJ[ID_COMBINATION];
    BU_ALLOC(intern->idb_ptr, struct rt_comb_internal);

    comb = (struct rt_comb_internal *)intern->idb_ptr;
    RT_COMB_INTERNAL_INIT(comb);
    bu_vls_init(&comb->shader);
    bu_vls_init(&comb->material);
}





static union tree *
facetize_region_end(struct db_tree_state *tsp,
                    const struct db_full_path *pathp,
                    union tree *curtree,
                    void *client_data)
{
    union tree **facetize_tree;

    if (tsp) RT_CK_DBTS(tsp);
    if (pathp) RT_CK_FULL_PATH(pathp);

    facetize_tree = (union tree **)client_data;

    if (curtree->tr_op == OP_NOP) return curtree;

    if (*facetize_tree) {
        union tree *tr;
        BU_ALLOC(tr, union tree);
        RT_TREE_INIT(tr);
        tr->tr_op = OP_UNION;
        tr->tr_b.tb_regionp = REGION_NULL;
        tr->tr_b.tb_left = *facetize_tree;
        tr->tr_b.tb_right = curtree;
        *facetize_tree = tr;
    } else {
        *facetize_tree = curtree;
    }

    /* Tree has been saved, and will be freed later */
    return TREE_NULL;
}



/**
 * Execute NMG tessellation routines on the tree to produce an explicitly
 * bounded, evaluated version of the geometry.
 *
 * Although (to my knowledge) there aren't any real world scenarios requiring a
 * current rt_db_internal that persists after its dbip is invalid, there is no
 * API guarantee that such a rt_db_internal couldn't be created and this routine
 * WILL fail catastrophically if the database info in rt_comb_internal is not
 * current.  ONLY use comb methods on rt_db_internals when they are associated
 * with a current, valid dbip.
 */
int
rt_comb_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct bg_tess_tol *ttol, const struct bn_tol *tol)
{
    if (!r || !m  || !ip || !ttol || !tol)
	return BRLCAD_ERROR;

    struct bu_list *vlfree = &rt_vlfree;
   /* validate it's a comb */
    if (ip->idb_type != ID_COMBINATION) bu_bomb("rt_comb_tess() type not ID_COMBINATION");
    struct rt_comb_internal *comb = (struct rt_comb_internal *)ip->idb_ptr;
    RT_CK_COMB(comb);
    RT_CK_DBI(comb->src_dbip);
    if (!comb->src_objname) {
	bu_log("rt_comb_tess() - comb object name not defined in rt_comb_internal, cannot perform database db_walk_tree");
	return BRLCAD_ERROR;
    }

    int i = 0;
    int failed = 0;
    union tree *facetize_tree = (union tree *)0;
    struct db_tree_state init_state;
    db_init_db_tree_state(&init_state, (struct db_i *)comb->src_dbip, &rt_uniresource);

    /* Establish tolerances */
    init_state.ts_ttol = ttol;
    init_state.ts_tol = tol;
    init_state.ts_m = &m;


    const char *argv[2];
    argv[0] = comb->src_objname;
    argv[1] = NULL;


    if (!BU_SETJUMP) {
        /* try */
        i = db_walk_tree((struct db_i *)comb->src_dbip, 1, (const char **)argv,
                         1,
                        &init_state,
                         0,                     /* take all regions */
                         facetize_region_end,
                         rt_booltree_leaf_tess,
                         (void *)&facetize_tree
                        );
    } else {
        /* catch */
        BU_UNSETJUMP;
        return BRLCAD_ERROR;
    } BU_UNSETJUMP;

    if (i < 0)
        return BRLCAD_ERROR;

    if (facetize_tree) {
        if (!BU_SETJUMP) {
            /* try */
            failed = nmg_boolean(facetize_tree, m, vlfree, tol, &rt_uniresource);
        } else {
            /* catch */
            BU_UNSETJUMP;
            return BRLCAD_ERROR;
        } BU_UNSETJUMP;

    } else {
        failed = 1;
    }

    if (!failed && facetize_tree) {
        NMG_CK_REGION(facetize_tree->tr_d.td_r);
        facetize_tree->tr_d.td_r = (struct nmgregion *)NULL;
    }

    if (facetize_tree) {
        db_free_tree(facetize_tree, &rt_uniresource);
    }

    return (failed) ? BRLCAD_ERROR : BRLCAD_OK;
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
