/*                        S K E T C H . C
 * BRL-CAD
 *
 * Copyright (c) 1990-2025 United States Government as represented by
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
/** @addtogroup primitives */
/** @{ */
/** @file primitives/sketch/sketch.c
 *
 * Provide support for 2D sketches.
 *
 */
/** @} */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "bnetwork.h"

#include "vmath.h"
#include "bu/debug.h"
#include "bu/cv.h"
#include "bu/opt.h"
#include "rt/db4.h"
#include "nmg.h"
#include "rt/geom.h"
#include "raytrace.h"

#include "../../librt_private.h"


extern void get_indices(void *seg, int *start, int *end);	/* from g_extrude.c */


int
rt_check_curve(const struct rt_curve *crv, const struct rt_sketch_internal *skt, int noisy)
{
    size_t i, j;
    int ret=0;

    /* empty sketches are invalid */
    if (crv->count == 0) {
	if (noisy)
	    bu_log("sketch is empty\n");
	return 1;
    }

    for (i=0; i<crv->count; i++) {
	const struct line_seg *lsg;
	const struct carc_seg *csg;
	const struct nurb_seg *nsg;
	const struct bezier_seg *bsg;
	const uint32_t *lng;

	lng = (uint32_t *)crv->segment[i];

	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		lsg = (struct line_seg *)lng;
		if ((size_t)lsg->start >= skt->vert_count ||
		    (size_t)lsg->end >= skt->vert_count)
		    ret++;
		break;
	    case CURVE_CARC_MAGIC:
		csg = (struct carc_seg *)lng;
		if ((size_t)csg->start >= skt->vert_count ||
		    (size_t)csg->end >= skt->vert_count)
		    ret++;
		break;
	    case CURVE_NURB_MAGIC:
		nsg = (struct nurb_seg *)lng;
		for (j=0; j<(size_t)nsg->c_size; j++) {
		    if ((size_t)nsg->ctl_points[j] >= skt->vert_count) {
			ret++;
			break;
		    }
		}
		break;
	    case CURVE_BEZIER_MAGIC:
		bsg = (struct bezier_seg *)lng;
		for (j=0; j<=(size_t)bsg->degree; j++) {
		    if ((size_t)bsg->ctl_points[j] >= skt->vert_count) {
			ret++;
			break;
		    }
		}
		break;
	    default:
		ret++;
		if (noisy)
		    bu_log("Unrecognized segment type in sketch\n");
		break;
	}
    }

    if (ret && noisy)
	bu_log("sketch references non-existent vertices!\n");
    return ret;
}


/**
 * Given a pointer to a GED database record, and a transformation
 * matrix, determine if this is a valid SKETCH, and if so, precompute
 * various terms of the formula.
 *
 * Returns -
 * 0 SKETCH is OK
 * !0 Error in description
 *
 * Implicit return -
 * A struct sketch_specific is created, and its address is
 * stored in stp->st_specific for use by sketch_shot().
 */
int
rt_sketch_prep(struct soltab *stp, struct rt_db_internal *ip, struct rt_i *rtip)
{
    if (!stp)
	return -1;
    RT_CK_SOLTAB(stp);
    if (ip) RT_CK_DB_INTERNAL(ip);
    if (rtip) RT_CK_RTI(rtip);

    stp->st_specific = (void *)NULL;
    return 0;
}


void
rt_sketch_print(const struct soltab *stp)
{
    if (stp) RT_CK_SOLTAB(stp);
}


/**
 * Intersect a ray with a sketch.  If an intersection occurs, a struct
 * seg will be acquired and filled in.
 *
 * Returns -
 * 0 MISS
 * >0 HIT
 */
int
rt_sketch_shot(struct soltab *stp, struct xray *rp, struct application *ap, struct seg *seghead)
{
    if (!stp || !rp || !ap || !seghead)
	return 0;

    RT_CK_SOLTAB(stp);
    RT_CK_RAY(rp);
    RT_CK_APPLICATION(ap);

    /* can't hit 'em as they're not solid geometry, so no surfno, not
     * hit point, nada.
     */

    return 0;			/* MISS */
}


/**
 * Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_sketch_norm(struct hit *hitp, struct soltab *stp, struct xray *rp)
{
    if (!hitp || !rp)
	return;

    RT_CK_HIT(hitp);
    if (stp) RT_CK_SOLTAB(stp);
    RT_CK_RAY(rp);

    VJOIN1(hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir);
}


/**
 * Return the curvature of the sketch.
 */
void
rt_sketch_curve(struct curvature *cvp, struct hit *hitp, struct soltab *stp)
{
    if (!cvp || !hitp)
	return;

    RT_CK_HIT(hitp);
    if (stp) RT_CK_SOLTAB(stp);

    cvp->crv_c1 = cvp->crv_c2 = 0;

    /* any tangent direction */
    bn_vec_ortho(cvp->crv_pdir, hitp->hit_normal);
}


/**
 * For a hit on the surface of an sketch, return the (u, v) coordinates
 * of the hit point, 0 <= u, v <= 1.
 *
 * u = azimuth,  v = elevation
 */
void
rt_sketch_uv(struct application *ap, struct soltab *stp, struct hit *hitp, struct uvcoord *uvp)
{
    if (ap) RT_CK_APPLICATION(ap);
    if (stp) RT_CK_SOLTAB(stp);
    if (hitp) RT_CK_HIT(hitp);
    if (!uvp)
	return;
}


void
rt_sketch_free(struct soltab *stp)
{
    if (stp) RT_CK_SOLTAB(stp);
}


/*
  determine whether the 2D point 'pt' is within the bounds of the sketch
  by counting the number of intersections between the curve and a line
  starting at the given point pointing horizontally away from the y-axis.

  if the number of intersections is odd, the point is within the sketch,
  and if it's even, the point is outside the sketch.

  return 1 if point 'pt' is within the sketch, 0 if not.
*/
int
rt_sketch_contains(struct rt_sketch_internal *sk, point2d_t pt)
{
    fastf_t nseg;
    int i, hits;

    uint32_t *lng;
    struct line_seg *lsg;
    struct carc_seg *csg;

    point2d_t pt1, pt2, isec;
    vect2d_t one, two;

    nseg = sk->curve.count;
    hits = 0;
    isec[Y] = pt[Y];

    for (i=0; i<nseg; i++) {
	lng = (uint32_t *)sk->curve.segment[i];

	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		lsg = (struct line_seg *)lng;
		V2MOVE(pt1, sk->verts[lsg->start]);
		V2MOVE(pt2, sk->verts[lsg->end]);
		if (pt[Y] > FMAX(pt1[Y], pt2[Y]) || pt[Y] < FMIN(pt1[Y], pt2[Y])) {
		    continue;
		}
		isec[X] = pt1[X] + (isec[Y] - pt1[Y]) * ((pt1[X] - pt2[X]) / (pt1[Y] - pt2[Y]));
		if ((pt[X] >= 0 && pt[X] < isec[X]) || (pt[X] < 0 && pt[X] > isec[X])) {
		    hits++;
		}
		break;
	    case CURVE_CARC_MAGIC:
		csg = (struct carc_seg *)lng;
		V2MOVE(pt1, sk->verts[csg->start]);
		V2MOVE(pt2, sk->verts[csg->end]);
		if (csg->radius <=0.0) {
		    vect2d_t r, dist;
		    V2SUB2(r, pt2, pt1);
		    V2SUB2(dist, r, pt);
		    if (MAG2SQ(dist) < MAG2SQ(r)) hits++;
		} else {
		    vect2d_t r, dist, center;
		    V2SUB2(dist, pt2, pt1);
		    if (csg->center_is_left) {
			r[X] = -dist[Y];
			r[Y] = dist[X];
		    } else {
			r[X] = dist[Y];
			r[Y] = -dist[X];
		    }
		    V2SCALE(dist, dist, 0.5);
		    V2SCALE(r, r, sqrt((csg->radius*csg->radius - MAG2SQ(dist))/(MAG2SQ(r))));
		    V2ADD3(center, sk->verts[csg->start], dist, r);
		    V2SUB2(dist, center, pt);
		    if (MAG2SQ(dist) > (csg->radius*csg->radius)) {
			/* outside the circle - if it passes between the endpoints, count it as a hit */
			/* otherwise, it will hit either twice or not at all, and can be ignored */
			if (pt[Y] > FMIN(pt1[Y], pt2[Y]) && pt[Y] <= FMAX(pt1[Y], pt2[Y])) {
			    if ((pt[X] >= 0 && pt[X] < FMIN(pt1[X], pt2[X]))
				|| (pt[X] < 0 && pt[X] > FMAX(pt1[X], pt2[X]))) hits++;
			}
		    } else {
			fastf_t angMin, angMax, angle;
			V2SUB2(one, pt1, center);
			V2SUB2(two, pt2, center);
			if (csg->orientation == 0) {
			    /* ccw arc */
			    angMin = atan2(one[Y], one[X]);
			    angMax = atan2(two[Y], two[X]);
			} else {
			    angMax = atan2(one[Y], one[X]);
			    angMin = atan2(two[Y], two[X]);
			}
			if (pt[X] >= 0) {
			    angle = atan2(dist[Y], sqrt(csg->radius*csg->radius - dist[Y]*dist[Y]));
			} else {
			    angle = atan2(dist[Y], -sqrt(csg->radius*csg->radius - dist[Y]*dist[Y]));
			}
			angMax -= angMin;
			angle -= angMin;
			if (angMax < 0) angMax += M_2PI;
			if (angle < 0) angle += M_2PI;
			if (angle < angMax) hits++;
		    }
		}
		break;
	    case CURVE_BEZIER_MAGIC:
		break;
	    case CURVE_NURB_MAGIC:
		break;
	    default:
		bu_log("rt_revolve_prep: ERROR: unrecognized segment type!\n");
		break;
	}
    }
    return hits%2;
}


/* sets bounds to { XMIN, XMAX, YMIN, YMAX } */
void
rt_sketch_bounds(struct rt_sketch_internal *sk, fastf_t *bounds)
{
    fastf_t nseg, radius;
    int i, j;

    uint32_t *lng;
    struct line_seg *lsg;
    struct carc_seg *csg;
    struct nurb_seg *nsg;
    struct bezier_seg *bsg;

    nseg = sk->curve.count;
    bounds[0] = bounds[2] = FLT_MAX;
    bounds[1] = bounds[3] = -FLT_MAX;

    for (i=0; i<nseg; i++) {
	lng = (uint32_t *)sk->curve.segment[i];

	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		lsg = (struct line_seg *)lng;
		V_MIN(bounds[0], FMIN(sk->verts[lsg->start][X], sk->verts[lsg->end][X]));
		V_MAX(bounds[1], FMAX(sk->verts[lsg->start][X], sk->verts[lsg->end][X]));
		V_MIN(bounds[2], FMIN(sk->verts[lsg->start][Y], sk->verts[lsg->end][Y]));
		V_MAX(bounds[3], FMAX(sk->verts[lsg->start][Y], sk->verts[lsg->end][Y]));
		break;
	    case CURVE_CARC_MAGIC:
		csg = (struct carc_seg *)lng;
		if (csg->radius <= 0.0) {
		    /* full circle, use center +- radius */
		    vect2d_t r;
		    V2SUB2(r, sk->verts[csg->end], sk->verts[csg->start]);
		    radius = sqrt(MAG2SQ(r));
		    V_MIN(bounds[0], sk->verts[csg->end][X] - radius);
		    V_MAX(bounds[1], sk->verts[csg->end][X] + radius);
		    V_MIN(bounds[2], sk->verts[csg->end][Y] - radius);
		    V_MAX(bounds[3], sk->verts[csg->end][Y] + radius);
		} else {
		    /* TODO: actually calculate values from carc center */
		    V_MIN(bounds[0], FMIN(sk->verts[csg->start][X], sk->verts[csg->end][X]) - 2*csg->radius);
		    V_MAX(bounds[1], FMAX(sk->verts[csg->start][X], sk->verts[csg->end][X]) + 2*csg->radius);
		    V_MIN(bounds[2], FMIN(sk->verts[csg->start][Y], sk->verts[csg->end][Y]) - 2*csg->radius);
		    V_MAX(bounds[3], FMAX(sk->verts[csg->start][Y], sk->verts[csg->end][Y]) + 2*csg->radius);
		}
		break;
	    case CURVE_BEZIER_MAGIC:
		bsg = (struct bezier_seg *)lng;
		for (j=0; j<=bsg->degree; j++) {
		    V_MIN(bounds[0], sk->verts[bsg->ctl_points[j]][X]);
		    V_MAX(bounds[1], sk->verts[bsg->ctl_points[j]][X]);
		    V_MIN(bounds[2], sk->verts[bsg->ctl_points[j]][Y]);
		    V_MAX(bounds[3], sk->verts[bsg->ctl_points[j]][Y]);
		}
		break;
	    case CURVE_NURB_MAGIC:
		nsg = (struct nurb_seg *)lng;
		for (j=0; j < nsg->c_size; j++) {
		    V_MIN(bounds[0], sk->verts[nsg->ctl_points[j]][X]);
		    V_MAX(bounds[1], sk->verts[nsg->ctl_points[j]][X]);
		    V_MIN(bounds[2], sk->verts[nsg->ctl_points[j]][Y]);
		    V_MAX(bounds[3], sk->verts[nsg->ctl_points[j]][Y]);
		}
		break;
	    default:
		bu_log("rt_revolve_prep: ERROR: unrecognized segment type!\n");
		break;
	}
    }

}


int
rt_sketch_degree(struct rt_sketch_internal *sk)
{

    uint32_t *lng;
    struct nurb_seg *nsg;
    struct bezier_seg *bsg;
    int nseg;
    int degree;
    int i;

    degree = 0;
    nseg = sk->curve.count;

    for (i=0; i<nseg; i++) {
	lng = (uint32_t *)sk->curve.segment[i];

	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		if (degree < 1) degree = 1;
		break;
	    case CURVE_CARC_MAGIC:
		if (degree < 2) degree = 2;
		break;
	    case CURVE_BEZIER_MAGIC:
		bsg = (struct bezier_seg *)lng;
		if (degree < bsg->degree) degree = bsg->degree;
		break;
	    case CURVE_NURB_MAGIC:
		nsg = (struct nurb_seg *)lng;
		if (degree < nsg->order+1) degree = nsg->order+1;
		break;
	    default:
		bu_log("rt_revolve_prep: ERROR: unrecognized segment type!\n");
		break;
	}
    }
    return degree;
}


int
seg_to_vlist(struct bu_list *vlfree, struct bu_list *vhead, const struct bg_tess_tol *ttol, fastf_t *V, fastf_t *u_vec, fastf_t *v_vec, struct rt_sketch_internal *sketch_ip, void *seg)
{
    int ret=0;
    int i;
    uint32_t *lng;
    struct line_seg *lsg;
    struct carc_seg *csg;
    struct nurb_seg *nsg;
    struct bezier_seg *bsg;
    fastf_t delta;
    point_t center, start_pt;
    fastf_t pt[4];
    vect_t semi_a, semi_b;
    fastf_t radius;
    vect_t norm;

    BU_CK_LIST_HEAD(vhead);

    VSETALL(semi_a, 0);
    VSETALL(semi_b, 0);
    VSETALL(center, 0);

    lng = (uint32_t *)seg;
    switch (*lng) {
	case CURVE_LSEG_MAGIC:
	    lsg = (struct line_seg *)lng;
	    if ((size_t)lsg->start >= sketch_ip->vert_count || (size_t)lsg->end >= sketch_ip->vert_count) {
		ret++;
		break;
	    }
	    VJOIN2(pt, V, sketch_ip->verts[lsg->start][0], u_vec, sketch_ip->verts[lsg->start][1], v_vec);
	    BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_MOVE);
	    VJOIN2(pt, V, sketch_ip->verts[lsg->end][0], u_vec, sketch_ip->verts[lsg->end][1], v_vec);
	    BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_DRAW);
	    break;
	case CURVE_CARC_MAGIC:
	    {
		point2d_t mid_pt, start2d, end2d, center2d, s2m, dir;
		fastf_t s2m_len_sq, len_sq, tmp_len, cross_z;
		fastf_t start_ang, end_ang, tot_ang, cosdel, sindel;
		fastf_t oldu, oldv, newu, newv;
		int nsegs;

		csg = (struct carc_seg *)lng;
		if ((size_t)csg->start >= sketch_ip->vert_count || (size_t)csg->end >= sketch_ip->vert_count) {
		    ret++;
		    break;
		}

		delta = M_PI_4;
		if (csg->radius <= 0.0) {
		    VJOIN2(center, V, sketch_ip->verts[csg->end][0], u_vec, sketch_ip->verts[csg->end][1], v_vec);
		    VJOIN2(pt, V, sketch_ip->verts[csg->start][0], u_vec, sketch_ip->verts[csg->start][1], v_vec);
		    VSUB2(semi_a, pt, center);
		    VCROSS(norm, u_vec, v_vec);
		    VCROSS(semi_b, norm, semi_a);
		    VUNITIZE(semi_b);
		    radius = MAGNITUDE(semi_a);
		    VSCALE(semi_b, semi_b, radius);
		} else if (csg->radius <= SMALL_FASTF) {
		    bu_log("Radius too small in sketch!\n");
		    break;
		} else {
		    radius = csg->radius;
		}

		if (ttol->abs > 0.0) {
		    fastf_t tmp_delta, ratio;

		    ratio = ttol->abs / radius;
		    if (ratio < 1.0) {
			tmp_delta = 2.0 * acos(1.0 - ratio);
			if (tmp_delta < delta)
			    delta = tmp_delta;
		    }
		}
		if (ttol->rel > 0.0 && ttol->rel < 1.0) {
		    fastf_t tmp_delta;

		    tmp_delta = 2.0 * acos(1.0 - ttol->rel);
		    if (tmp_delta < delta)
			delta = tmp_delta;
		}
		if (ttol->norm > 0.0) {
		    fastf_t normal;

		    normal = ttol->norm * DEG2RAD;
		    if (normal < delta)
			delta = normal;
		}
		if (csg->radius <= 0.0) {
		    /* this is a full circle */
		    nsegs = ceil(M_2PI / delta);
		    delta = M_2PI / (double)nsegs;
		    cosdel = cos(delta);
		    sindel = sin(delta);
		    oldu = 1.0;
		    oldv = 0.0;
		    VJOIN2(start_pt, center, oldu, semi_a, oldv, semi_b);
		    BV_ADD_VLIST(vlfree, vhead, start_pt, BV_VLIST_LINE_MOVE);
		    for (i=1; i<nsegs; i++) {
			newu = oldu * cosdel - oldv * sindel;
			newv = oldu * sindel + oldv * cosdel;
			VJOIN2(pt, center, newu, semi_a, newv, semi_b);
			BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_DRAW);
			oldu = newu;
			oldv = newv;
		    }
		    BV_ADD_VLIST(vlfree, vhead, start_pt, BV_VLIST_LINE_DRAW);
		    break;
		}

		/* this is an arc (not a full circle) */
		V2MOVE(start2d, sketch_ip->verts[csg->start]);
		V2MOVE(end2d, sketch_ip->verts[csg->end]);
		mid_pt[0] = (start2d[0] + end2d[0]) * 0.5;
		mid_pt[1] = (start2d[1] + end2d[1]) * 0.5;
		V2SUB2(s2m, mid_pt, start2d);
		dir[0] = -s2m[1];
		dir[1] = s2m[0];
		s2m_len_sq =  s2m[0]*s2m[0] + s2m[1]*s2m[1];
		if (s2m_len_sq <= SMALL_FASTF) {
		    bu_log("start and end points are too close together in circular arc of sketch\n");
		    break;
		}
		len_sq = radius*radius - s2m_len_sq;
		if (len_sq < 0.0) {
		    bu_log("Impossible radius for specified start and end points in circular arc\n");
		    break;
		}
		tmp_len = sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
		dir[0] = dir[0] / tmp_len;
		dir[1] = dir[1] / tmp_len;
		tmp_len = sqrt(len_sq);
		V2JOIN1(center2d, mid_pt, tmp_len, dir);

		/* check center location */
		cross_z = (end2d[X] - start2d[X])*(center2d[Y] - start2d[Y]) -
		    (end2d[Y] - start2d[Y])*(center2d[X] - start2d[X]);
		if (!(cross_z > 0.0 && csg->center_is_left))
		    V2JOIN1(center2d, mid_pt, -tmp_len, dir);
		start_ang = atan2(start2d[Y]-center2d[Y], start2d[X]-center2d[X]);
		end_ang = atan2(end2d[Y]-center2d[Y], end2d[X]-center2d[X]);
		if (csg->orientation) {
		    /* clock-wise */
		    while (end_ang > start_ang)
			end_ang -= M_2PI;
		} else {
		    /* counter-clock-wise */
		    while (end_ang < start_ang)
			end_ang += M_2PI;
		}
		tot_ang = end_ang - start_ang;
		nsegs = ceil(tot_ang / delta);
		if (nsegs < 0)
		    nsegs = -nsegs;
		if (nsegs < 3)
		    nsegs = 3;
		delta = tot_ang / nsegs;
		cosdel = cos(delta);
		sindel = sin(delta);
		VJOIN2(center, V, center2d[0], u_vec, center2d[1], v_vec);
		VJOIN2(start_pt, V, start2d[0], u_vec, start2d[1], v_vec);
		oldu = (start2d[0] - center2d[0]);
		oldv = (start2d[1] - center2d[1]);
		BV_ADD_VLIST(vlfree, vhead, start_pt, BV_VLIST_LINE_MOVE);
		for (i=0; i<nsegs; i++) {
		    newu = oldu * cosdel - oldv * sindel;
		    newv = oldu * sindel + oldv * cosdel;
		    VJOIN2(pt, center, newu, u_vec, newv, v_vec);
		    BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_DRAW);
		    oldu = newu;
		    oldv = newv;
		}
		break;
	    }
	case CURVE_NURB_MAGIC:
	    {
		struct edge_g_cnurb eg;
		int coords;
		fastf_t inv_weight;
		int num_intervals;
		fastf_t param_delta, epsilon;

		nsg = (struct nurb_seg *)lng;
		for (i=0; i<nsg->c_size; i++) {
		    if ((size_t)nsg->ctl_points[i] >= sketch_ip->vert_count) {
			ret++;
			break;
		    }
		}
		if (nsg->order < 3) {
		    /* just straight lines */
		    VJOIN2(start_pt, V, sketch_ip->verts[nsg->ctl_points[0]][0], u_vec, sketch_ip->verts[nsg->ctl_points[0]][1], v_vec);
		    if (RT_NURB_IS_PT_RATIONAL(nsg->pt_type)) {
			inv_weight = 1.0/nsg->weights[0];
			VSCALE(start_pt, start_pt, inv_weight);
		    }
		    BV_ADD_VLIST(vlfree, vhead, start_pt, BV_VLIST_LINE_MOVE);
		    for (i=1; i<nsg->c_size; i++) {
			VJOIN2(pt, V, sketch_ip->verts[nsg->ctl_points[i]][0], u_vec, sketch_ip->verts[nsg->ctl_points[i]][1], v_vec);
			if (RT_NURB_IS_PT_RATIONAL(nsg->pt_type)) {
			    inv_weight = 1.0/nsg->weights[i];
			    VSCALE(pt, pt, inv_weight);
			}
			BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_DRAW);
		    }
		    break;
		}
		eg.l.magic = NMG_EDGE_G_CNURB_MAGIC;
		eg.order = nsg->order;
		eg.k.k_size = nsg->k.k_size;
		eg.k.knots = nsg->k.knots;
		eg.c_size = nsg->c_size;
		coords = 3 + RT_NURB_IS_PT_RATIONAL(nsg->pt_type);
		eg.pt_type = RT_NURB_MAKE_PT_TYPE(coords, 2, RT_NURB_IS_PT_RATIONAL(nsg->pt_type));
		eg.ctl_points = (fastf_t *)bu_malloc(nsg->c_size * coords * sizeof(fastf_t), "eg.ctl_points");
		if (RT_NURB_IS_PT_RATIONAL(nsg->pt_type)) {
		    for (i=0; i<nsg->c_size; i++) {
			VJOIN2(&eg.ctl_points[i*coords], V, sketch_ip->verts[nsg->ctl_points[i]][0], u_vec, sketch_ip->verts[nsg->ctl_points[i]][1], v_vec);
			eg.ctl_points[(i+1)*coords - 1] = nsg->weights[i];
		    }
		} else {
		    for (i=0; i<nsg->c_size; i++)
			VJOIN2(&eg.ctl_points[i*coords], V, sketch_ip->verts[nsg->ctl_points[i]][0], u_vec, sketch_ip->verts[nsg->ctl_points[i]][1], v_vec);
		}
		epsilon = MAX_FASTF;
		if (ttol->abs > 0.0 && ttol->abs < epsilon)
		    epsilon = ttol->abs;
		if (ttol->rel > 0.0) {
		    point2d_t min_pt, max_pt, tmp_pt;
		    point2d_t diff;
		    fastf_t tmp_epsilon;

		    min_pt[0] = MAX_FASTF;
		    min_pt[1] = MAX_FASTF;
		    max_pt[0] = -MAX_FASTF;
		    max_pt[1] = -MAX_FASTF;

		    for (i=0; i<nsg->c_size; i++) {
			V2MOVE(tmp_pt, sketch_ip->verts[nsg->ctl_points[i]]);
			if (tmp_pt[0] > max_pt[0])
			    max_pt[0] = tmp_pt[0];
			if (tmp_pt[1] > max_pt[1])
			    max_pt[1] = tmp_pt[1];
			if (tmp_pt[0] < min_pt[0])
			    min_pt[0] = tmp_pt[0];
			if (tmp_pt[1] < min_pt[1])
			    min_pt[1] = tmp_pt[1];
		    }

		    V2SUB2(diff, max_pt, min_pt);
		    tmp_epsilon = ttol->rel * sqrt(MAG2SQ(diff));
		    if (tmp_epsilon < epsilon)
			epsilon = tmp_epsilon;

		}
		param_delta = rt_cnurb_par_edge(&eg, epsilon);
		num_intervals = ceil((nsg->k.knots[nsg->k.k_size-1] - nsg->k.knots[0])/param_delta);
		if (num_intervals < 3)
		    num_intervals = 3;
		if (num_intervals > 500) {
		    bu_log("num_intervals was %d, clamped to 500\n", num_intervals);
		    num_intervals = 500;
		}
		param_delta = (nsg->k.knots[nsg->k.k_size-1] - nsg->k.knots[0])/(double)num_intervals;
		for (i=0; i<=num_intervals; i++) {
		    fastf_t t;
		    int j;

		    t = nsg->k.knots[0] + i*param_delta;
		    nmg_nurb_c_eval(&eg, t, pt);
		    if (RT_NURB_IS_PT_RATIONAL(nsg->pt_type)) {
			for (j=0; j<coords-1; j++)
			    pt[j] /= pt[coords-1];
		    }
		    if (i == 0)
			BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_MOVE);
		    else
			BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_DRAW);
		}
		bu_free((char *)eg.ctl_points, "eg.ctl_points");
		break;
	    }
	case CURVE_BEZIER_MAGIC: {
	    struct bezier_2d_list *bezier_hd, *bz;
	    fastf_t epsilon;

	    bsg = (struct bezier_seg *)lng;

	    for (i=0; i<=bsg->degree; i++) {
		if ((size_t)bsg->ctl_points[i] >= sketch_ip->vert_count) {
		    ret++;
		    break;
		}
	    }

	    if (bsg->degree < 1) {
		bu_log("g_sketch: ERROR: Bezier curve with illegal degree (%d)\n",
		       bsg->degree);
		ret++;
		break;
	    }

	    if (bsg->degree == 1) {
		/* straight line */
		VJOIN2(start_pt, V, sketch_ip->verts[bsg->ctl_points[0]][0],
		       u_vec, sketch_ip->verts[bsg->ctl_points[0]][1], v_vec);
		BV_ADD_VLIST(vlfree, vhead, start_pt, BV_VLIST_LINE_MOVE);
		for (i=1; i<=bsg->degree; i++) {
		    VJOIN2(pt, V, sketch_ip->verts[bsg->ctl_points[i]][0],
			   u_vec, sketch_ip->verts[bsg->ctl_points[i]][1], v_vec);
		    BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_DRAW);
		}
		break;
	    }

	    /* use tolerance to determine coarseness of plot */
	    epsilon = MAX_FASTF;
	    if (ttol->abs > 0.0 && ttol->abs < epsilon)
		epsilon = ttol->abs;
	    if (ttol->rel > 0.0) {
		point2d_t min_pt, max_pt, tmp_pt;
		point2d_t diff;
		fastf_t tmp_epsilon;

		min_pt[0] = MAX_FASTF;
		min_pt[1] = MAX_FASTF;
		max_pt[0] = -MAX_FASTF;
		max_pt[1] = -MAX_FASTF;

		for (i=0; i<=bsg->degree; i++) {
		    V2MOVE(tmp_pt, sketch_ip->verts[bsg->ctl_points[i]]);
		    if (tmp_pt[0] > max_pt[0])
			max_pt[0] = tmp_pt[0];
		    if (tmp_pt[1] > max_pt[1])
			max_pt[1] = tmp_pt[1];
		    if (tmp_pt[0] < min_pt[0])
			min_pt[0] = tmp_pt[0];
		    if (tmp_pt[1] < min_pt[1])
			min_pt[1] = tmp_pt[1];
		}

		V2SUB2(diff, max_pt, min_pt);
		tmp_epsilon = ttol->rel * sqrt(MAG2SQ(diff));
		if (tmp_epsilon < epsilon)
		    epsilon = tmp_epsilon;

	    }


	    /* Create an initial bezier_2d_list */
	    BU_ALLOC(bezier_hd, struct bezier_2d_list);

	    BU_LIST_INIT(&bezier_hd->l);
	    bezier_hd->ctl = (point2d_t *)bu_calloc(bsg->degree + 1, sizeof(point2d_t),
						    "g_sketch.c: bezier_hd->ctl");
	    for (i=0; i<=bsg->degree; i++) {
		V2MOVE(bezier_hd->ctl[i], sketch_ip->verts[bsg->ctl_points[i]]);
	    }

	    /* now do subdivision as necessary */
	    bezier_hd = bezier_subdivide(bezier_hd, bsg->degree, epsilon, 0);

	    /* plot the results */
	    bz = BU_LIST_FIRST(bezier_2d_list, &bezier_hd->l);
	    VJOIN2(pt, V, bz->ctl[0][0], u_vec, bz->ctl[0][1], v_vec);
	    BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_MOVE);

	    while (BU_LIST_WHILE(bz, bezier_2d_list, &(bezier_hd->l))) {
		BU_LIST_DEQUEUE(&bz->l);
		for (i=1; i<=bsg->degree; i++) {
		    VJOIN2(pt, V, bz->ctl[i][0], u_vec,
			   bz->ctl[i][1], v_vec);
		    BV_ADD_VLIST(vlfree, vhead, pt, BV_VLIST_LINE_DRAW);
		}
		bu_free((char *)bz->ctl, "g_sketch.c: bz->ctl");
		bu_free((char *)bz, "g_sketch.c: bz");
	    }
	    bu_free((char *)bezier_hd, "g_sketch.c: bezier_hd");
	    break;
	}
	default:
	    bu_log("seg_to_vlist: ERROR: unrecognized segment type!\n");
	    break;
    }

    return ret;
}


int
curve_to_vlist(struct bu_list *vlfree, struct bu_list *vhead, const struct bg_tess_tol *ttol, point_t V, vect_t u_vec, vect_t v_vec, struct rt_sketch_internal *sketch_ip, struct rt_curve *crv)
{
    size_t seg_no;
    int ret=0;

    BU_CK_LIST_HEAD(vhead);

    for (seg_no=0; seg_no < crv->count; seg_no++) {
	ret += seg_to_vlist(vlfree, vhead, ttol, V, u_vec, v_vec, sketch_ip, crv->segment[seg_no]);
    }

    return ret;
}


int
rt_sketch_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct bg_tess_tol *ttol, const struct bn_tol *UNUSED(tol), const struct bview *UNUSED(info))
{
    struct rt_sketch_internal *sketch_ip;
    int ret;
    int myret=0;

    BU_CK_LIST_HEAD(vhead);
    RT_CK_DB_INTERNAL(ip);
    sketch_ip = (struct rt_sketch_internal *)ip->idb_ptr;
    RT_SKETCH_CK_MAGIC(sketch_ip);
    struct bu_list *vlfree = &rt_vlfree;

    ret=curve_to_vlist(vlfree, vhead, ttol, sketch_ip->V, sketch_ip->u_vec, sketch_ip->v_vec, sketch_ip, &sketch_ip->curve);
    if (ret) {
	myret--;
	bu_log("WARNING: Errors in sketch (%d segments reference non-existent vertices)\n",
	       ret);
    }

    return myret;
}

/**
 * Given a sketch, bounds, and precision for rt_sketch_centroid, check
 * if the precision is large enough (if the sample size is small
 * enough) to intersect every curve of the given sketch.
 *
 * Returns -
 * 0 precision is sufficiently large
 * !0 precision is too small
 */
static int
sketch_centroid_check_precision(const struct rt_sketch_internal *skt,
				   fastf_t precision,
				   fastf_t bounds[4])
{
    fastf_t x_inc, y_inc, min_inc;
    fastf_t min_size = INFINITY;
    unsigned int i;
    const struct rt_curve *crv = &skt->curve;
    x_inc = (bounds[1]-bounds[0])/precision;
    y_inc = (bounds[3]-bounds[2])/precision;
    min_inc = x_inc < y_inc ? x_inc : y_inc;
    for (i = 0; i < crv->count; i++) {
	point2d_t size;
	const struct line_seg *lsg;
	const struct carc_seg *csg;
	const struct nurb_seg *nsg;
	const struct bezier_seg *bsg;
	const uint32_t *lng;
	int j;
	point2d_t min_pt, max_pt;

	V2SETALL(min_pt, INFINITY);
	V2SETALL(max_pt, -INFINITY);

	lng = (uint32_t *)crv->segment[i];

	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		lsg = (struct line_seg *)lng;
		V2SUB2(size, skt->verts[lsg->end], skt->verts[lsg->start]);
		V_MIN(min_size, MAGNITUDE2(size));
		break;
	    case CURVE_CARC_MAGIC:
		csg = (struct carc_seg *)lng;
		V2SUB2(size, skt->verts[csg->end], skt->verts[csg->start]);
		V_MIN(min_size, MAGNITUDE2(size));
		break;
	    case CURVE_NURB_MAGIC:
		nsg = (struct nurb_seg *)lng;
		for (j = 0; j < nsg->c_size; j++) {
		    V2MINMAX(min_pt, max_pt, skt->verts[nsg->ctl_points[j]]);
		}
		V2SUB2(size, max_pt, min_pt);
		V_MIN(min_size, MAGNITUDE2(size));
		break;
	    case CURVE_BEZIER_MAGIC:
		bsg = (struct bezier_seg *)lng;
		for (j = 0; j <= bsg->degree; j++) {
		    V2MINMAX(min_pt, max_pt, skt->verts[bsg->ctl_points[j]]);
		}
		V2SUB2(size, max_pt, min_pt);
		V_MIN(min_size, MAGNITUDE2(size));
		break;
	}
    }
    return (min_size < min_inc);
}

static long
sketch_update_centroid_with_coords(point_t *cent,
				   struct rt_sketch_internal *skt,
				   long n,
				   fastf_t i,
				   fastf_t j)
{
    point2d_t pt;
    point_t pt3d;
    V2SET(pt, i, j);
    VSET(pt3d, i, j, 0);
    if (rt_sketch_contains(skt, pt)) {
	VADD2(*cent, *cent, pt3d);
	n++;
    }
    return n;
}


static long
sketch_centroid_with_precision(point_t *cent,
			       struct rt_sketch_internal *skt,
			       fastf_t precision,
			       long n,
			       fastf_t bounds[4])
{
    fastf_t x_inc, y_inc;
    fastf_t x_skip, y_skip;
    fastf_t i, j;

    x_inc = (bounds[1] - bounds[0])/precision;
    y_inc = (bounds[3] - bounds[2])/precision;

    x_skip = 2 * x_inc;
    y_skip = 2 * y_inc;

    if (n > 0) {
	VSCALE(*cent, *cent, n);
	for (i = bounds[0] + x_inc; i < bounds[1]; i += x_skip) {
	    for (j = bounds[2]; j < bounds[3]; j += y_inc) {
		n = sketch_update_centroid_with_coords(cent, skt, n, i, j);
	    }
	}
	for (i = bounds[0]; i < bounds[1]; i += x_skip) {
	    for (j = bounds[2] + y_inc; j < bounds[3]; j += y_skip) {
		n = sketch_update_centroid_with_coords(cent, skt, n, i, j);
	    }
	}
    } else {
	for (i = bounds[0]; i < bounds[1]; i += x_inc) {
	    for (j = bounds[2]; j < bounds[3]; j += y_inc) {
		n = sketch_update_centroid_with_coords(cent, skt, n, i, j);
	    }
	}
    }
    VSCALE(*cent, *cent, 1.0 / (fastf_t)n);
    return n;
}

void
rt_sketch_centroid(point_t *cent, const struct rt_db_internal *ip)
{
    /* With BN_TOL_DIST instead, even relatively simple sketches can
     * bounce back and forth between points < 0.005 apart but > 0.0005
     * for a long time.  Since calculating is very computationally
     * intensive, using this TOLERANCE instead makes the computation
     * finish significantly faster without a huge loss of accuracy
     */
    const fastf_t TOLERANCE = 0.005;
    /* This is the initial precision that will be used as the
     * precision for the first iteration, assuming that the sketch
     * does not have any features that are small enough to escape
     * detection with this precision.  1024 was chosen because
     * smaller values tend to be very inaccurate, and oscillate enough
     * that convergence is not achieved; 1024 is a good balance
     * between (for the majority of sketches) being too large (and
     * missing a smaller convergence) and being too small (and doing
     * extra iterations for no reason).
     */
    fastf_t precision = 1024;
    fastf_t bounds[4];
    struct rt_sketch_internal *sketch_ip = (struct rt_sketch_internal *)ip->idb_ptr;
    point_t current_cents[2];
    point_t difference = {INFINITY, INFINITY, INFINITY};
    long n[2] = {0, 0};
    int i = 1;
    RT_SKETCH_CK_MAGIC(sketch_ip);

    VSETALL(*cent, 0.0);
    VSETALL(current_cents[0], 0.0);
    VSETALL(current_cents[1], 0.0);

    rt_sketch_bounds(sketch_ip, bounds);
    while (sketch_centroid_check_precision(sketch_ip, precision, bounds)) {
	precision *= 2;
    }

    while (!NEAR_ZERO(MAGNITUDE(difference), TOLERANCE)) {
	n[i] = sketch_centroid_with_precision(&current_cents[i], sketch_ip, precision, n[i], bounds);
	precision *= M_SQRT2;
	VSUB2(difference, current_cents[i], current_cents[!i]);
	i = !i;
    }

    VMOVE(*cent, current_cents[!i]);
}


/**
 * Returns -
 * -1 failure
 * 0 OK.  *r points to nmgregion that holds this tessellation.
 */
int
rt_sketch_tess(struct nmgregion **UNUSED(r), struct model *UNUSED(m), struct rt_db_internal *ip, const struct bg_tess_tol *UNUSED(ttol), const struct bn_tol *UNUSED(tol))
{
    if (ip) RT_CK_DB_INTERNAL(ip);

    return -1;
}


/**
 * Import an SKETCH from the database format to the internal format.
 * Apply modeling transformations as well.
 */
int
rt_sketch_import4(struct rt_db_internal *ip, const struct bu_external *ep, const fastf_t *mat, const struct db_i *dbip)
{
    struct rt_sketch_internal *sketch_ip;
    union record *rp;
    size_t seg_no;
    unsigned char *ptr;
    struct rt_curve *crv;
    size_t i;

    /* must be double for import and export */
    double v[ELEMENTS_PER_VECT];
    double *vp;

    if (dbip) RT_CK_DBI(dbip);

    BU_CK_EXTERNAL(ep);
    rp = (union record *)ep->ext_buf;
    /* Check record type */
    if (rp->u_id != DBID_SKETCH) {
	bu_log("rt_sketch_import4: defective record\n");
	return -1;
    }

    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_SKETCH;
    ip->idb_meth = &OBJ[ID_SKETCH];
    BU_ALLOC(ip->idb_ptr, struct rt_sketch_internal);

    sketch_ip = (struct rt_sketch_internal *)ip->idb_ptr;
    sketch_ip->magic = RT_SKETCH_INTERNAL_MAGIC;

    if (mat == NULL) mat = bn_mat_identity;
    bu_cv_ntohd((unsigned char *)v, rp->skt.skt_V, ELEMENTS_PER_VECT);
    MAT4X3PNT(sketch_ip->V, mat, v);
    bu_cv_ntohd((unsigned char *)v, rp->skt.skt_uvec, ELEMENTS_PER_VECT);
    MAT4X3VEC(sketch_ip->u_vec, mat, v);
    bu_cv_ntohd((unsigned char *)v, rp->skt.skt_vvec, ELEMENTS_PER_VECT);
    MAT4X3VEC(sketch_ip->v_vec, mat, v);
    sketch_ip->vert_count = ntohl(*(uint32_t *)rp->skt.skt_vert_count);
    sketch_ip->curve.count = ntohl(*(uint32_t *)rp->skt.skt_count);

    ptr = (unsigned char *)rp;
    ptr += sizeof(struct sketch_rec);
    if (sketch_ip->vert_count) {
	sketch_ip->verts = (point2d_t *)bu_calloc(sketch_ip->vert_count, sizeof(point2d_t), "sketch_ip->vert");
	vp = (double *)bu_calloc(sketch_ip->vert_count, sizeof(double)*ELEMENTS_PER_VECT2D, "vp");
	bu_cv_ntohd((unsigned char *)vp, ptr, sketch_ip->vert_count*ELEMENTS_PER_VECT2D);

	/* convert double to fastf_t */
	for (i=0; i<sketch_ip->vert_count; i++) {
	    sketch_ip->verts[i][X] = vp[(i*ELEMENTS_PER_VECT2D)+0];
	    sketch_ip->verts[i][Y] = vp[(i*ELEMENTS_PER_VECT2D)+1];
	}

	bu_free(vp, "vp");
	ptr += 16 * sketch_ip->vert_count;
    }

    if (sketch_ip->curve.count)
	sketch_ip->curve.segment = (void **)bu_calloc(sketch_ip->curve.count, sizeof(void *), "segs");
    else
	sketch_ip->curve.segment = (void **)NULL;
    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	uint32_t magic;
	struct line_seg *lsg;
	struct carc_seg *csg;
	struct nurb_seg *nsg;
	struct bezier_seg *bsg;

	/* must be double for import and export */
	double scan;
	double *scanp;

	magic = ntohl(*(uint32_t *)ptr);
	ptr += SIZEOF_NETWORK_LONG;
	switch (magic) {
	    case CURVE_LSEG_MAGIC:
		BU_ALLOC(lsg, struct line_seg);
		lsg->magic = magic;
		lsg->start = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		lsg->end = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		sketch_ip->curve.segment[seg_no] = (void *)lsg;
		break;
	    case CURVE_CARC_MAGIC:
		BU_ALLOC(csg, struct carc_seg);
		csg->magic = magic;
		csg->start = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		csg->end = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		csg->orientation = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		csg->center_is_left = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		bu_cv_ntohd((unsigned char *)&scan, ptr, 1);
		csg->radius = scan; /* convert double to fastf_t */
		ptr += SIZEOF_NETWORK_DOUBLE;
		sketch_ip->curve.segment[seg_no] = (void *)csg;
		break;
	    case CURVE_NURB_MAGIC:
		BU_ALLOC(nsg, struct nurb_seg);
		nsg->magic = magic;
		nsg->order = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		nsg->pt_type = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		nsg->k.k_size = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;

		nsg->k.knots = (fastf_t *)bu_malloc(nsg->k.k_size * sizeof(fastf_t), "nsg->k.knots");
		scanp = (double *)bu_malloc(nsg->k.k_size * sizeof(double), "scanp");
		bu_cv_ntohd((unsigned char *)scanp, ptr, nsg->k.k_size);

		/* convert double to fastf_t */
		for (i=0; i<(size_t)nsg->k.k_size; i++) {
		    nsg->k.knots[i] = scanp[i];
		}
		bu_free(scanp, "scanp");

		ptr += SIZEOF_NETWORK_DOUBLE * nsg->k.k_size;
		nsg->c_size = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		nsg->ctl_points = (int *)bu_malloc(nsg->c_size * sizeof(int), "nsg->ctl_points");
		for (i=0; i<(size_t)nsg->c_size; i++) {
		    nsg->ctl_points[i] = ntohl(*(uint32_t *)ptr);
		    ptr += SIZEOF_NETWORK_LONG;
		}
		if (RT_NURB_IS_PT_RATIONAL(nsg->pt_type)) {
		    nsg->weights = (fastf_t *)bu_malloc(nsg->c_size * sizeof(fastf_t), "nsg->weights");
		    scanp = (double *)bu_malloc(nsg->c_size * sizeof(double), "scanp");
		    bu_cv_ntohd((unsigned char *)scanp, ptr, nsg->c_size);

		    /* convert double to fastf_t */
		    for (i=0; i<(size_t)nsg->k.k_size; i++) {
			nsg->weights[i] = scanp[i];
		    }
		    bu_free(scanp, "scanp");

		    ptr += SIZEOF_NETWORK_DOUBLE * nsg->c_size;
		} else {
		    nsg->weights = (fastf_t *)NULL;
		}
		sketch_ip->curve.segment[seg_no] = (void *)nsg;
		break;
	    case CURVE_BEZIER_MAGIC:
		BU_ALLOC(bsg, struct bezier_seg);
		bsg->magic = magic;
		bsg->degree = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		bsg->ctl_points = (int *)bu_calloc(bsg->degree + 1, sizeof(int), "bsg->ctl_points");
		for (i=0; i<=(size_t)bsg->degree; i++) {
		    bsg->ctl_points[i] = ntohl(*(uint32_t *)ptr);
		    ptr += SIZEOF_NETWORK_LONG;
		}
		sketch_ip->curve.segment[seg_no] = (void *)bsg;
		break;
	    default:
		bu_bomb("rt_sketch_import4: ERROR: unrecognized segment type!\n");
		break;
	}
    }

    crv = &sketch_ip->curve;

    if (crv->count)
	crv->reverse = (int *)bu_calloc(crv->count, sizeof(int), "crv->reverse");
    for (i=0; i<crv->count; i++) {
	crv->reverse[i] = ntohl(*(uint32_t *)ptr);
	ptr += SIZEOF_NETWORK_LONG;
    }

    return 0;			/* OK */
}


/**
 * The name is added by the caller, in the usual place.
 */
int
rt_sketch_export4(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_sketch_internal *sketch_ip;
    union record *rec;
    size_t i, seg_no, nbytes=0, ngran;
    unsigned char *ptr;

    /* must be double for import and export */
    double tmp_vec[ELEMENTS_PER_VECT];

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_SKETCH) return -1;
    sketch_ip = (struct rt_sketch_internal *)ip->idb_ptr;
    RT_SKETCH_CK_MAGIC(sketch_ip);

    BU_CK_EXTERNAL(ep);

    nbytes = sizeof(union record);		/* base record */
    nbytes += sketch_ip->vert_count*(8*2);	/* vertex list */

    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	uint32_t *lng;
	struct nurb_seg *nseg;
	struct bezier_seg *bseg;

	lng = (uint32_t *)sketch_ip->curve.segment[seg_no];
	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		nbytes += 12;
		break;
	    case CURVE_CARC_MAGIC:
		nbytes += 28;
		break;
	    case CURVE_NURB_MAGIC:
		nseg = (struct nurb_seg *)lng;
		nbytes += 16 + sizeof(struct knot_vector) + nseg->k.k_size * 8 + nseg->c_size * 4;
		if (RT_NURB_IS_PT_RATIONAL(nseg->pt_type))
		    nbytes += nseg->c_size * 8;	/* weights */
		break;
	    case CURVE_BEZIER_MAGIC:
		bseg = (struct bezier_seg *)lng;
		nbytes += 8 + (bseg->degree + 1) * 4;
		break;
	    default:
		bu_log("rt_sketch_export4: unsupported segment type (x%x)\n", *lng);
		bu_bomb("rt_sketch_export4: unsupported segment type\n");
	}
    }

    ngran = ceil((double)(nbytes + sizeof(union record)) / sizeof(union record));
    ep->ext_nbytes = ngran * sizeof(union record);
    ep->ext_buf = (uint8_t *)bu_calloc(1, ep->ext_nbytes, "sketch external");

    rec = (union record *)ep->ext_buf;

    rec->skt.skt_id = DBID_SKETCH;

    /* convert from local editing units to mm and export
     * to database record format
     */
    VSCALE(tmp_vec, sketch_ip->V, local2mm);
    bu_cv_htond(rec->skt.skt_V, (unsigned char *)tmp_vec, ELEMENTS_PER_VECT);

    /* uvec and uvec are unit vectors, do not convert to/from mm */
    VMOVE(tmp_vec, sketch_ip->u_vec); /* convert fastf_t to double */
    bu_cv_htond(rec->skt.skt_uvec, (unsigned char *)tmp_vec, ELEMENTS_PER_VECT);
    VMOVE(tmp_vec, sketch_ip->v_vec); /* convert fastf_t to double */
    bu_cv_htond(rec->skt.skt_vvec, (unsigned char *)tmp_vec, ELEMENTS_PER_VECT);

    *(uint32_t *)rec->skt.skt_vert_count = htonl(sketch_ip->vert_count);
    *(uint32_t *)rec->skt.skt_count = htonl(sketch_ip->curve.count);
    *(uint32_t *)rec->skt.skt_count = htonl(ngran-1);

    ptr = (unsigned char *)rec;
    ptr += sizeof(struct sketch_rec);
    /* convert 2D points to mm */
    for (i=0; i<sketch_ip->vert_count; i++) {
	/* must be double for import and export */
	double pt2d[ELEMENTS_PER_VECT2D];

	V2SCALE(pt2d, sketch_ip->verts[i], local2mm);
	bu_cv_htond(ptr, (const unsigned char *)pt2d, ELEMENTS_PER_VECT2D);
	ptr += 16;
    }

    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	struct line_seg *lseg;
	struct carc_seg *cseg;
	struct nurb_seg *nseg;
	struct bezier_seg *bseg;
	uint32_t *lng;

	/* must be double for import and export */
	double tmp_fastf;
	double *scanp;

	/* write segment type ID, and segment parameters */
	lng = (uint32_t *)sketch_ip->curve.segment[seg_no];
	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		lseg = (struct line_seg *)lng;
		*(uint32_t *)ptr = htonl(CURVE_LSEG_MAGIC);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(lseg->start);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(lseg->end);
		ptr += SIZEOF_NETWORK_LONG;
		break;
	    case CURVE_CARC_MAGIC:
		cseg = (struct carc_seg *)lng;
		*(uint32_t *)ptr = htonl(CURVE_CARC_MAGIC);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(cseg->start);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(cseg->end);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(cseg->orientation);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(cseg->center_is_left);
		ptr += SIZEOF_NETWORK_LONG;
		tmp_fastf = cseg->radius * local2mm;
		bu_cv_htond(ptr, (unsigned char *)&tmp_fastf, 1);
		ptr += SIZEOF_NETWORK_DOUBLE;
		break;
	    case CURVE_NURB_MAGIC:
		nseg = (struct nurb_seg *)lng;
		*(uint32_t *)ptr = htonl(CURVE_NURB_MAGIC);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(nseg->order);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(nseg->pt_type);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(nseg->k.k_size);
		ptr += SIZEOF_NETWORK_LONG;
		scanp = (double *)bu_malloc(nseg->k.k_size * sizeof(double), "scanp");
		/* convert fastf_t to double */
		for (i=0; i<(size_t)nseg->k.k_size; i++) {
		    scanp[i] = nseg->k.knots[i];
		}
		bu_cv_htond(ptr, (const unsigned char *)scanp, nseg->k.k_size);
		bu_free(scanp, "scanp");
		ptr += nseg->k.k_size * 8;
		*(uint32_t *)ptr = htonl(nseg->c_size);
		ptr += SIZEOF_NETWORK_LONG;
		for (i=0; i<(size_t)nseg->c_size; i++) {
		    *(uint32_t *)ptr = htonl(nseg->ctl_points[i]);
		    ptr += SIZEOF_NETWORK_LONG;
		}
		if (RT_NURB_IS_PT_RATIONAL(nseg->pt_type)) {
		    scanp = (double *)bu_malloc(nseg->c_size * sizeof(double), "scanp");
		    /* convert fastf_t to double */
		    for (i=0; i<(size_t)nseg->c_size; i++) {
			scanp[i] = nseg->weights[i];
		    }
		    bu_cv_htond(ptr, (const unsigned char *)scanp, nseg->c_size);
		    bu_free(scanp, "scanp");
		    ptr += SIZEOF_NETWORK_DOUBLE * nseg->c_size;
		}
		break;
	    case CURVE_BEZIER_MAGIC:
		bseg = (struct bezier_seg *)lng;
		*(uint32_t *)ptr = htonl(CURVE_BEZIER_MAGIC);
		ptr += SIZEOF_NETWORK_LONG;
		*(uint32_t *)ptr = htonl(bseg->degree);
		ptr += SIZEOF_NETWORK_LONG;
		for (i=0; i<=(size_t)bseg->degree; i++) {
		    *(uint32_t *)ptr = htonl(bseg->ctl_points[i]);
		    ptr += SIZEOF_NETWORK_LONG;
		}
		break;
	    default:
		bu_bomb("rt_sketch_export4: ERROR: unrecognized curve type!\n");
		break;

	}
    }

    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	*(uint32_t *)ptr = htonl(sketch_ip->curve.reverse[seg_no]);
	ptr += SIZEOF_NETWORK_LONG;
    }

    return 0;
}

int
rt_sketch_mat(struct rt_db_internal *rop, const mat_t mat, const struct rt_db_internal *ip)
{
    if (!rop || !ip || !mat)
	return BRLCAD_OK;

    struct rt_sketch_internal *tip = (struct rt_sketch_internal *)ip->idb_ptr;
    RT_SKETCH_CK_MAGIC(tip);
    struct rt_sketch_internal *top = (struct rt_sketch_internal *)rop->idb_ptr;
    RT_SKETCH_CK_MAGIC(top);

    vect_t V, u_vec, v_vec;
    VMOVE(V, tip->V);
    VMOVE(u_vec, tip->u_vec);
    VMOVE(v_vec, tip->v_vec);
    MAT4X3PNT(top->V, mat, V);
    MAT4X3VEC(top->u_vec, mat, u_vec);
    MAT4X3VEC(top->v_vec, mat, v_vec);

    return BRLCAD_OK;
}


/**
 * Import an SKETCH from the database format to the internal format.
 * Apply modeling transformations as well.
 */
int
rt_sketch_import5(struct rt_db_internal *ip, const struct bu_external *ep, const fastf_t *mat, const struct db_i *dbip)
{
    struct rt_sketch_internal *sketch_ip;
    size_t seg_no;
    unsigned char *ptr;
    struct rt_curve *crv;
    size_t i;

    /* must be double for import and export */
    double v[ELEMENTS_PER_VECT];
    double *vp;

    if (dbip) RT_CK_DBI(dbip);
    BU_CK_EXTERNAL(ep);

    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_SKETCH;
    ip->idb_meth = &OBJ[ID_SKETCH];
    BU_ALLOC(ip->idb_ptr, struct rt_sketch_internal);

    sketch_ip = (struct rt_sketch_internal *)ip->idb_ptr;
    sketch_ip->magic = RT_SKETCH_INTERNAL_MAGIC;

    ptr = ep->ext_buf;
    bu_cv_ntohd((unsigned char *)v, ptr, ELEMENTS_PER_VECT);
    VMOVE(sketch_ip->V, v);
    ptr += SIZEOF_NETWORK_DOUBLE * ELEMENTS_PER_VECT;
    bu_cv_ntohd((unsigned char *)v, ptr, ELEMENTS_PER_VECT);
    VMOVE(sketch_ip->u_vec, v);
    ptr += SIZEOF_NETWORK_DOUBLE * ELEMENTS_PER_VECT;
    bu_cv_ntohd((unsigned char *)v, ptr, ELEMENTS_PER_VECT);
    VMOVE(sketch_ip->v_vec, v);
    ptr += SIZEOF_NETWORK_DOUBLE * ELEMENTS_PER_VECT;
    sketch_ip->vert_count = ntohl(*(uint32_t *)ptr);
    ptr += SIZEOF_NETWORK_LONG;
    sketch_ip->curve.count = ntohl(*(uint32_t *)ptr);
    ptr += SIZEOF_NETWORK_LONG;

    /* Apply transform */
    if (mat == NULL) mat = bn_mat_identity;
    rt_sketch_mat(ip, mat, ip);

    if (sketch_ip->vert_count) {
	sketch_ip->verts = (point2d_t *)bu_calloc(sketch_ip->vert_count, sizeof(point2d_t), "sketch_ip->vert");
	vp = (double *)bu_calloc(sketch_ip->vert_count, sizeof(double)*ELEMENTS_PER_VECT2D, "vp");
	bu_cv_ntohd((unsigned char *)vp, ptr, sketch_ip->vert_count*2);

	/* convert double to fastf_t */
	for (i=0; i<sketch_ip->vert_count; i++) {
	    sketch_ip->verts[i][X] = vp[(i*ELEMENTS_PER_VECT2D)+0];
	    sketch_ip->verts[i][Y] = vp[(i*ELEMENTS_PER_VECT2D)+1];
	}

	bu_free(vp, "vp");
	ptr += SIZEOF_NETWORK_DOUBLE * 2 * sketch_ip->vert_count;
    }

    if (sketch_ip->curve.count)
	sketch_ip->curve.segment = (void **)bu_calloc(sketch_ip->curve.count, sizeof(void *), "segs");
    else
	sketch_ip->curve.segment = (void **)NULL;
    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	uint32_t magic;
	struct line_seg *lsg;
	struct carc_seg *csg;
	struct nurb_seg *nsg;
	struct bezier_seg *bsg;

	/* must be double for import and export */
	double scan;
	double *scanp;

	magic = ntohl(*(uint32_t *)ptr);
	ptr += SIZEOF_NETWORK_LONG;
	switch (magic) {
	    case CURVE_LSEG_MAGIC:
		BU_ALLOC(lsg, struct line_seg);
		lsg->magic = magic;
		lsg->start = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		lsg->end = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		sketch_ip->curve.segment[seg_no] = (void *)lsg;
		break;
	    case CURVE_CARC_MAGIC:
		BU_ALLOC(csg, struct carc_seg);
		csg->magic = magic;
		csg->start = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		csg->end = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		csg->orientation = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		csg->center_is_left = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		bu_cv_ntohd((unsigned char *)&scan, ptr, 1);
		csg->radius = scan; /* double to fastf_t */
		ptr += SIZEOF_NETWORK_DOUBLE;
		sketch_ip->curve.segment[seg_no] = (void *)csg;
		break;
	    case CURVE_NURB_MAGIC:
		BU_ALLOC(nsg, struct nurb_seg);
		nsg->magic = magic;
		nsg->order = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		nsg->pt_type = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		nsg->k.k_size = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;

		nsg->k.knots = (fastf_t *)bu_malloc(nsg->k.k_size * sizeof(fastf_t), "nsg->k.knots");
		scanp = (double *)bu_malloc(nsg->k.k_size * sizeof(double), "scanp");
		bu_cv_ntohd((unsigned char *)scanp, ptr, nsg->k.k_size);

		/* convert double to fastf_t */
		for (i=0; i<(size_t)nsg->k.k_size; i++) {
		    nsg->k.knots[i] = scanp[i];
		}
		bu_free(scanp, "scanp");

		ptr += SIZEOF_NETWORK_DOUBLE * nsg->k.k_size;
		nsg->c_size = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		nsg->ctl_points = (int *)bu_malloc(nsg->c_size * sizeof(int), "nsg->ctl_points");
		for (i=0; i<(size_t)nsg->c_size; i++) {
		    nsg->ctl_points[i] = ntohl(*(uint32_t *)ptr);
		    ptr += SIZEOF_NETWORK_LONG;
		}
		if (RT_NURB_IS_PT_RATIONAL(nsg->pt_type)) {
		    nsg->weights = (fastf_t *)bu_malloc(nsg->c_size * sizeof(fastf_t), "nsg->weights");
		    scanp = (double *)bu_malloc(nsg->c_size * sizeof(double), "scanp");
		    bu_cv_ntohd((unsigned char *)scanp, ptr, nsg->c_size);

		    /* convert double to fastf_t */
		    for (i=0; i<(size_t)nsg->k.k_size; i++) {
			nsg->weights[i] = scanp[i];
		    }
		    bu_free(scanp, "scanp");

		    ptr += SIZEOF_NETWORK_DOUBLE * nsg->c_size;
		} else
		    nsg->weights = (fastf_t *)NULL;
		sketch_ip->curve.segment[seg_no] = (void *)nsg;
		break;
	    case CURVE_BEZIER_MAGIC:
		BU_ALLOC(bsg, struct bezier_seg);
		bsg->magic = magic;
		bsg->degree = ntohl(*(uint32_t *)ptr);
		ptr += SIZEOF_NETWORK_LONG;
		bsg->ctl_points = (int *)bu_calloc(bsg->degree+1, sizeof(int), "bsg->ctl_points");
		for (i=0; i<=(size_t)bsg->degree; i++) {
		    bsg->ctl_points[i] = ntohl(*(uint32_t *)ptr);
		    ptr += SIZEOF_NETWORK_LONG;
		}
		sketch_ip->curve.segment[seg_no] = (void *)bsg;
		break;
	    default:
		bu_bomb("rt_sketch_import4: ERROR: unrecognized segment type!\n");
		break;
	}
    }

    crv = &sketch_ip->curve;

    if (crv->count) {
	crv->reverse = (int *)bu_calloc(crv->count, sizeof(int), "crv->reverse");
    }

    for (i=0; i<crv->count; i++) {
	crv->reverse[i] = ntohl(*(uint32_t *)ptr);
	ptr += SIZEOF_NETWORK_LONG;
    }

    return 0;			/* OK */
}


/**
 * The name is added by the caller, in the usual place.
 */
int
rt_sketch_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_sketch_internal *sketch_ip;
    unsigned char *cp;
    size_t seg_no;
    size_t i;

    /* must be double for import and export */
    double tmp_vec[ELEMENTS_PER_VECT];

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_SKETCH) return -1;
    sketch_ip = (struct rt_sketch_internal *)ip->idb_ptr;
    RT_SKETCH_CK_MAGIC(sketch_ip);

    BU_CK_EXTERNAL(ep);

    /* tally up size of buffer needed */
    ep->ext_nbytes = 3 * (ELEMENTS_PER_VECT * SIZEOF_NETWORK_DOUBLE)	/* V, u_vec, v_vec */
	+ 2 * SIZEOF_NETWORK_LONG		/* vert_count and count */
	+ 2 * sketch_ip->vert_count * SIZEOF_NETWORK_DOUBLE	/* 2D-vertices */
	+ sketch_ip->curve.count * SIZEOF_NETWORK_LONG;	/* reverse flags */

    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	uint32_t *lng;
	struct nurb_seg *nseg;
	struct bezier_seg *bseg;

	lng = (uint32_t *)sketch_ip->curve.segment[seg_no];
	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		/* magic + start + end */
		ep->ext_nbytes += 3 * SIZEOF_NETWORK_LONG;
		break;
	    case CURVE_CARC_MAGIC:
		/* magic + start + end + orientation + center_is_left + (double)radius*/
		ep->ext_nbytes += 5 * SIZEOF_NETWORK_LONG + SIZEOF_NETWORK_DOUBLE;
		break;
	    case CURVE_NURB_MAGIC:
		nseg = (struct nurb_seg *)lng;
		/* magic + order + pt_type + c_size */
		ep->ext_nbytes += 4 * SIZEOF_NETWORK_LONG;
		/* (double)knots */
		ep->ext_nbytes += SIZEOF_NETWORK_LONG + nseg->k.k_size * SIZEOF_NETWORK_DOUBLE;
		/* control point count */
		ep->ext_nbytes += nseg->c_size * SIZEOF_NETWORK_LONG;
		if (RT_NURB_IS_PT_RATIONAL(nseg->pt_type))
		    /* (double)weights */
		    ep->ext_nbytes += nseg->c_size * SIZEOF_NETWORK_DOUBLE;
		break;
	    case CURVE_BEZIER_MAGIC:
		bseg = (struct bezier_seg *)lng;
		/* magic + degree */
		ep->ext_nbytes += 2 * SIZEOF_NETWORK_LONG;
		/* control points */
		ep->ext_nbytes += (bseg->degree + 1) * SIZEOF_NETWORK_LONG;
		break;
	    default:
		bu_log("rt_sketch_export5: unsupported segment type (x%x)\n", *lng);
		bu_bomb("rt_sketch_export5: unsupported segment type\n");
	}
    }
    ep->ext_buf = (uint8_t *)bu_malloc(ep->ext_nbytes, "sketch external");

    cp = (unsigned char *)ep->ext_buf;

    /* scale and export */
    VSCALE(tmp_vec, sketch_ip->V, local2mm);
    bu_cv_htond(cp, (unsigned char *)tmp_vec, ELEMENTS_PER_VECT);
    cp += ELEMENTS_PER_VECT * SIZEOF_NETWORK_DOUBLE;

    /* uvec and uvec are unit vectors, do not convert to/from mm */
    VMOVE(tmp_vec, sketch_ip->u_vec); /* convert fastf_t to double */
    bu_cv_htond(cp, (unsigned char *)tmp_vec, ELEMENTS_PER_VECT);
    cp += ELEMENTS_PER_VECT * SIZEOF_NETWORK_DOUBLE;
    VMOVE(tmp_vec, sketch_ip->v_vec); /* convert fastf_t to double */
    bu_cv_htond(cp, (unsigned char *)tmp_vec, ELEMENTS_PER_VECT);
    cp += ELEMENTS_PER_VECT * SIZEOF_NETWORK_DOUBLE;

    *(uint32_t *)cp = htonl(sketch_ip->vert_count);
    cp += SIZEOF_NETWORK_LONG;
    *(uint32_t *)cp = htonl(sketch_ip->curve.count);
    cp += SIZEOF_NETWORK_LONG;

    /* convert 2D points to mm */
    for (i=0; i<sketch_ip->vert_count; i++) {
	/* must be double for import and export */
	double pt2d[ELEMENTS_PER_VECT2D];

	V2SCALE(pt2d, sketch_ip->verts[i], local2mm);
	bu_cv_htond(cp, (const unsigned char *)pt2d, ELEMENTS_PER_VECT2D);
	cp += 2 * SIZEOF_NETWORK_DOUBLE;
    }

    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	struct line_seg *lseg;
	struct carc_seg *cseg;
	struct nurb_seg *nseg;
	struct bezier_seg *bseg;
	uint32_t *lng;

	/* must be double for import and export */
	double scan;
	double *scanp;

	/* write segment type ID, and segment parameters */
	lng = (uint32_t *)sketch_ip->curve.segment[seg_no];
	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		lseg = (struct line_seg *)lng;
		*(uint32_t *)cp = htonl(CURVE_LSEG_MAGIC);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(lseg->start);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(lseg->end);
		cp += SIZEOF_NETWORK_LONG;
		break;
	    case CURVE_CARC_MAGIC:
		cseg = (struct carc_seg *)lng;
		*(uint32_t *)cp = htonl(CURVE_CARC_MAGIC);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(cseg->start);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(cseg->end);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(cseg->orientation);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(cseg->center_is_left);
		cp += SIZEOF_NETWORK_LONG;
		scan = cseg->radius * local2mm;
		bu_cv_htond(cp, (unsigned char *)&scan, 1);
		cp += SIZEOF_NETWORK_DOUBLE;
		break;
	    case CURVE_NURB_MAGIC:
		nseg = (struct nurb_seg *)lng;
		*(uint32_t *)cp = htonl(CURVE_NURB_MAGIC);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(nseg->order);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(nseg->pt_type);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(nseg->k.k_size);
		cp += SIZEOF_NETWORK_LONG;
		scanp = (double *)bu_malloc(nseg->k.k_size * sizeof(double), "scanp");
		/* convert fastf_t to double */
		for (i=0; i<(size_t)nseg->k.k_size; i++) {
		    scanp[i] = nseg->k.knots[i];
		}
		bu_cv_htond(cp, (const unsigned char *)nseg->k.knots, nseg->k.k_size);
		bu_free(scanp, "scanp");
		cp += nseg->k.k_size * SIZEOF_NETWORK_DOUBLE;
		*(uint32_t *)cp = htonl(nseg->c_size);
		cp += SIZEOF_NETWORK_LONG;
		for (i=0; i<(size_t)nseg->c_size; i++) {
		    *(uint32_t *)cp = htonl(nseg->ctl_points[i]);
		    cp += SIZEOF_NETWORK_LONG;
		}
		if (RT_NURB_IS_PT_RATIONAL(nseg->pt_type)) {
		    scanp = (double *)bu_malloc(nseg->c_size * sizeof(double), "scanp");
		    /* convert fastf_t to double */
		    for (i=0; i<(size_t)nseg->c_size; i++) {
			scanp[i] = nseg->weights[i];
		    }
		    bu_cv_htond(cp, (const unsigned char *)scanp, nseg->c_size);
		    bu_free(scanp, "scanp");
		    cp += SIZEOF_NETWORK_DOUBLE * nseg->c_size;
		}
		break;
	    case CURVE_BEZIER_MAGIC:
		bseg = (struct bezier_seg *)lng;
		*(uint32_t *)cp = htonl(CURVE_BEZIER_MAGIC);
		cp += SIZEOF_NETWORK_LONG;
		*(uint32_t *)cp = htonl(bseg->degree);
		cp += SIZEOF_NETWORK_LONG;
		for (i=0; i<=(size_t)bseg->degree; i++) {
		    *(uint32_t *)cp = htonl(bseg->ctl_points[i]);
		    cp += SIZEOF_NETWORK_LONG;
		}
		break;
	    default:
		bu_bomb("rt_sketch_export5: ERROR: unrecognized curve type!\n");
		break;

	}
    }

    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	*(uint32_t *)cp = htonl(sketch_ip->curve.reverse[seg_no]);
	cp += SIZEOF_NETWORK_LONG;
    }

    return 0;
}


/**
 * Make human-readable formatted presentation of this solid.  First
 * line describes type of solid.  Additional lines are indented one
 * tab, and give parameter values.
 */
int
rt_sketch_describe(struct bu_vls *str, const struct rt_db_internal *ip, int verbose, double mm2local)
{
    struct rt_sketch_internal *sketch_ip =
	(struct rt_sketch_internal *)ip->idb_ptr;
    size_t i;
    size_t seg_no;
    char buf[256];
    point_t V;
    vect_t u, v;

    RT_SKETCH_CK_MAGIC(sketch_ip);
    bu_vls_strcat(str, "2D sketch (SKETCH)\n");

    VSCALE(V, sketch_ip->V, mm2local);
    VSCALE(u, sketch_ip->u_vec, mm2local);
    VSCALE(v, sketch_ip->v_vec, mm2local);

    sprintf(buf, "\tV = (%g %g %g),  A = (%g %g %g), B = (%g %g %g)\n\t%lu vertices\n",
	    V3INTCLAMPARGS(V),
	    V3INTCLAMPARGS(u),
	    V3INTCLAMPARGS(v),
	    (long unsigned)sketch_ip->vert_count);
    bu_vls_strcat(str, buf);

    /* NOTE: this value is not arbitrary, but is dependent on the init in libged/list/list.c */
    if (verbose <= 99) {
	bu_vls_strcat(str, "    use -v (verbose) for all data\n");
	return 0;
    }

    if (sketch_ip->vert_count) {
	bu_vls_strcat(str, "\tVertices:\n\t");
	for (i=0; i<sketch_ip->vert_count; i++) {
	    sprintf(buf, " %lu-(%g %g)", (long unsigned)i, V2INTCLAMPARGS(sketch_ip->verts[i]));
	    bu_vls_strcat(str, buf);
	    if (i && (i+1)%3 == 0)
		bu_vls_strcat(str, "\n\t");
	}
    }
    bu_vls_strcat(str, "\n");

    sprintf(buf, "\n\tCurve:\n");
    bu_vls_strcat(str, buf);
    for (seg_no=0; seg_no < sketch_ip->curve.count; seg_no++) {
	struct line_seg *lsg;
	struct carc_seg *csg;
	struct nurb_seg *nsg;
	struct bezier_seg *bsg;

	lsg = (struct line_seg *)sketch_ip->curve.segment[seg_no];
	switch (lsg->magic) {
	    case CURVE_LSEG_MAGIC:
		lsg = (struct line_seg *)sketch_ip->curve.segment[seg_no];
		if ((size_t)lsg->start >= sketch_ip->vert_count || (size_t)lsg->end >= sketch_ip->vert_count) {
		    if (sketch_ip->curve.reverse[seg_no])
			sprintf(buf, "\t\tLine segment from vertex #%d to #%d\n",
				lsg->end, lsg->start);
		    else
			sprintf(buf, "\t\tLine segment from vertex #%d to #%d\n",
				lsg->start, lsg->end);
		} else {
		    if (sketch_ip->curve.reverse[seg_no])
			sprintf(buf, "\t\tLine segment (%g %g) <-> (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[lsg->end]),
				V2INTCLAMPARGS(sketch_ip->verts[lsg->start]));
		    else
			sprintf(buf, "\t\tLine segment (%g %g) <-> (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[lsg->start]),
				V2INTCLAMPARGS(sketch_ip->verts[lsg->end]));
		}
		bu_vls_strcat(str, buf);
		break;
	    case CURVE_CARC_MAGIC:
		csg = (struct carc_seg *)sketch_ip->curve.segment[seg_no];
		if (csg->radius < 0.0) {
		    bu_vls_strcat(str, "\t\tFull Circle:\n");

		    if ((size_t)csg->end >= sketch_ip->vert_count || (size_t)csg->start >= sketch_ip->vert_count) {
			sprintf(buf, "\t\tcenter at vertex #%d\n",
				csg->end);
			bu_vls_strcat(str, buf);
			sprintf(buf, "\t\tpoint on circle at vertex #%d\n",
				csg->start);
		    } else {
			sprintf(buf, "\t\t\tcenter: (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[csg->end]));
			bu_vls_strcat(str, buf);
			sprintf(buf, "\t\t\tpoint on circle: (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[csg->start]));
		    }
		    bu_vls_strcat(str, buf);
		} else {
		    bu_vls_strcat(str, "\t\tCircular Arc:\n");

		    if ((size_t)csg->end >= sketch_ip->vert_count || (size_t)csg->start >= sketch_ip->vert_count) {
			sprintf(buf, "\t\t\tstart at vertex #%d\n",
				csg->start);
			bu_vls_strcat(str, buf);
			sprintf(buf, "\t\t\tend at vertex #%d\n",
				csg->end);
			bu_vls_strcat(str, buf);
		    } else {
			sprintf(buf, "\t\t\tstart: (%g, %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[csg->start]));
			bu_vls_strcat(str, buf);
			sprintf(buf, "\t\t\tend: (%g, %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[csg->end]));
			bu_vls_strcat(str, buf);
		    }
		    sprintf(buf, "\t\t\tradius: %g\n", csg->radius*mm2local);
		    bu_vls_strcat(str, buf);
		    if (csg->orientation)
			bu_vls_strcat(str, "\t\t\tcurve is clock-wise\n");
		    else
			bu_vls_strcat(str, "\t\t\tcurve is counter-clock-wise\n");
		    if (csg->center_is_left)
			bu_vls_strcat(str, "\t\t\tcenter of curvature is left of the line from start point to end point\n");
		    else
			bu_vls_strcat(str, "\t\t\tcenter of curvature is right of the line from start point to end point\n");
		    if (sketch_ip->curve.reverse[seg_no])
			bu_vls_strcat(str, "\t\t\tarc is reversed\n");
		}
		break;
	    case CURVE_NURB_MAGIC:
		nsg = (struct nurb_seg *)sketch_ip->curve.segment[seg_no];
		bu_vls_strcat(str, "\t\tNURB Curve:\n");
		if (RT_NURB_IS_PT_RATIONAL(nsg->pt_type)) {
		    sprintf(buf, "\t\t\tCurve is rational\n");
		    bu_vls_strcat(str, buf);
		}
		sprintf(buf, "\t\t\torder = %d, number of control points = %d\n",
			nsg->order, nsg->c_size);
		bu_vls_strcat(str, buf);
		if ((size_t)nsg->ctl_points[0] >= sketch_ip->vert_count ||
		    (size_t)nsg->ctl_points[nsg->c_size-1] >= sketch_ip->vert_count) {
		    if (sketch_ip->curve.reverse[seg_no])
			sprintf(buf, "\t\t\tstarts at vertex #%d\n\t\t\tends at vertex #%d\n",
				nsg->ctl_points[nsg->c_size-1],
				nsg->ctl_points[0]);
		    else
			sprintf(buf, "\t\t\tstarts at vertex #%d\n\t\t\tends at vertex #%d\n",
				nsg->ctl_points[0],
				nsg->ctl_points[nsg->c_size-1]);
		} else {
		    if (sketch_ip->curve.reverse[seg_no])
			sprintf(buf, "\t\t\tstarts at (%g %g)\n\t\t\tends at (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[nsg->ctl_points[nsg->c_size-1]]),
				V2INTCLAMPARGS(sketch_ip->verts[nsg->ctl_points[0]]));
		    else
			sprintf(buf, "\t\t\tstarts at (%g %g)\n\t\t\tends at (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[nsg->ctl_points[0]]),
				V2INTCLAMPARGS(sketch_ip->verts[nsg->ctl_points[nsg->c_size-1]]));
		}
		bu_vls_strcat(str, buf);
		sprintf(buf, "\t\t\tknot values are %g to %g\n",
			INTCLAMP(nsg->k.knots[0]), INTCLAMP(nsg->k.knots[nsg->k.k_size-1]));
		bu_vls_strcat(str, buf);
		break;
	    case CURVE_BEZIER_MAGIC:
		bsg = (struct bezier_seg *)sketch_ip->curve.segment[seg_no];
		bu_vls_strcat(str, "\t\tBezier segment:\n");
		sprintf(buf, "\t\t\tdegree = %d\n", bsg->degree);
		bu_vls_strcat(str, buf);
		if ((size_t)bsg->ctl_points[0] >= sketch_ip->vert_count ||
		    (size_t)bsg->ctl_points[bsg->degree] >= sketch_ip->vert_count) {
		    if (sketch_ip->curve.reverse[seg_no]) {
			sprintf(buf, "\t\t\tstarts at vertex #%d\n\t\t\tends at vertex #%d\n",
				bsg->ctl_points[bsg->degree],
				bsg->ctl_points[0]);
		    } else {
			sprintf(buf, "\t\t\tstarts at vertex #%d\n\t\t\tends at vertex #%d\n",
				bsg->ctl_points[0],
				bsg->ctl_points[bsg->degree]);
		    }
		} else {
		    if (sketch_ip->curve.reverse[seg_no])
			sprintf(buf, "\t\t\tstarts at (%g %g)\n\t\t\tends at (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[bsg->ctl_points[bsg->degree]]),
				V2INTCLAMPARGS(sketch_ip->verts[bsg->ctl_points[0]]));
		    else
			sprintf(buf, "\t\t\tstarts at (%g %g)\n\t\t\tends at (%g %g)\n",
				V2INTCLAMPARGS(sketch_ip->verts[bsg->ctl_points[0]]),
				V2INTCLAMPARGS(sketch_ip->verts[bsg->ctl_points[bsg->degree]]));
		}
		bu_vls_strcat(str, buf);
		break;
	    default:
		bu_bomb("rt_sketch_describe: ERROR: unrecognized segment type\n");
	}
    }

    return 0;
}


void
rt_curve_free(struct rt_curve *crv)
{
    size_t i;

    if (crv->count)
	bu_free((char *)crv->reverse, "crv->reverse");
    for (i=0; i<crv->count; i++) {
	uint32_t *lng;
	struct nurb_seg *nsg;
	struct bezier_seg *bsg;

	lng = (uint32_t *)crv->segment[i];
	switch (*lng) {
	    case CURVE_NURB_MAGIC:
		nsg = (struct nurb_seg *)lng;
		bu_free((char *)nsg->ctl_points, "nsg->ctl_points");
		if (nsg->weights)
		    bu_free((char *)nsg->weights, "nsg->weights");
		bu_free((char *)nsg->k.knots, "nsg->k.knots");
		bu_free((char *)lng, "curve segment");
		break;
	    case CURVE_BEZIER_MAGIC:
		bsg = (struct bezier_seg *)lng;
		bu_free((char *)bsg->ctl_points, "bsg->ctl_points");
		bu_free((char *)lng, "curve segment");
		break;
	    case CURVE_LSEG_MAGIC:
	    case CURVE_CARC_MAGIC:
		bu_free((char *)lng, "curve segment");
		break;
	    default:
		bu_log("ERROR: rt_curve_free: unrecognized curve segments type!\n");
		break;
	}
    }

    if (crv->count > 0)
	bu_free((char *)crv->segment, "crv->segments");

    crv->count = 0;
    crv->reverse = (int *)NULL;
    crv->segment = (void **)NULL;
}


/**
 * Free the storage associated with the rt_db_internal version of this
 * solid.
 */
void
rt_sketch_ifree(struct rt_db_internal *ip)
{
    struct rt_sketch_internal *sketch_ip;
    struct rt_curve *crv;

    RT_CK_DB_INTERNAL(ip);

    sketch_ip = (struct rt_sketch_internal *)ip->idb_ptr;
    RT_SKETCH_CK_MAGIC(sketch_ip);
    sketch_ip->magic = 0;			/* sanity */

    if (sketch_ip->verts)
	bu_free((char *)sketch_ip->verts, "sketch_ip->verts");

    crv = &sketch_ip->curve;

    rt_curve_free(crv);

    bu_free((char *)sketch_ip, "sketch ifree");
    ip->idb_ptr = ((void *)0);	/* sanity */
}


void
rt_copy_curve(struct rt_curve *crv_out, const struct rt_curve *crv_in)
{
    size_t i, j;

    crv_out->count = crv_in->count;
    if (crv_out->count) {
	crv_out->reverse = (int *)bu_calloc(crv_out->count, sizeof(int), "crv->reverse");
	crv_out->segment = (void **)bu_calloc(crv_out->count, sizeof(void *), "crv->segments");
    }

    for (j=0; j<crv_out->count; j++) {
	uint32_t *lng;
	struct line_seg *lsg_out, *lsg_in;
	struct carc_seg *csg_out, *csg_in;
	struct nurb_seg *nsg_out, *nsg_in;
	struct bezier_seg *bsg_out, *bsg_in;

	crv_out->reverse[j] = (crv_in->reverse) ? crv_in->reverse[j] : 0;
	lng = (uint32_t *)crv_in->segment[j];
	switch (*lng) {
	    case CURVE_LSEG_MAGIC:
		lsg_in = (struct line_seg *)lng;
		BU_ALLOC(lsg_out, struct line_seg);
		crv_out->segment[j] = (void *)lsg_out;
		*lsg_out = *lsg_in;
		break;
	    case CURVE_CARC_MAGIC:
		csg_in = (struct carc_seg *)lng;
		BU_ALLOC(csg_out, struct carc_seg);
		crv_out->segment[j] = (void *)csg_out;
		*csg_out = *csg_in;
		break;
	    case CURVE_NURB_MAGIC:
		nsg_in = (struct nurb_seg *)lng;
		BU_ALLOC(nsg_out, struct nurb_seg);
		crv_out->segment[j] = (void *)nsg_out;
		*nsg_out = *nsg_in;
		nsg_out->ctl_points = (int *)bu_calloc(nsg_in->c_size, sizeof(int), "nsg_out->ctl_points");
		for (i=0; i<(size_t)nsg_out->c_size; i++)
		    nsg_out->ctl_points[i] = nsg_in->ctl_points[i];
		if (RT_NURB_IS_PT_RATIONAL(nsg_in->pt_type)) {
		    nsg_out->weights = (fastf_t *)bu_malloc(nsg_out->c_size * sizeof(fastf_t), "nsg_out->weights");
		    for (i=0; i<(size_t)nsg_out->c_size; i++)
			nsg_out->weights[i] = nsg_in->weights[i];
		} else
		    nsg_out->weights = (fastf_t *)NULL;
		nsg_out->k.knots = (fastf_t *)bu_malloc(nsg_in->k.k_size * sizeof(fastf_t), "nsg_out->k.knots");
		for (i=0; i<(size_t)nsg_in->k.k_size; i++)
		    nsg_out->k.knots[i] = nsg_in->k.knots[i];
		break;
	    case CURVE_BEZIER_MAGIC:
		bsg_in = (struct bezier_seg *)lng;
		BU_ALLOC(bsg_out, struct bezier_seg);
		crv_out->segment[j] = (void *)bsg_out;
		*bsg_out = *bsg_in;
		bsg_out->ctl_points = (int *)bu_calloc(bsg_out->degree + 1,
						       sizeof(int), "bsg_out->ctl_points");
		for (i=0; i<=(size_t)bsg_out->degree; i++) {
		    bsg_out->ctl_points[i] = bsg_in->ctl_points[i];
		}
		break;
	    default:
		bu_bomb("rt_copy_sketch: ERROR: unrecognized segment type!\n");
	}
    }

}


struct rt_sketch_internal *
rt_copy_sketch(const struct rt_sketch_internal *sketch_ip)
{
    struct rt_sketch_internal *out;
    size_t i;
    struct rt_curve *crv_out;

    RT_SKETCH_CK_MAGIC(sketch_ip);

    BU_ALLOC(out, struct rt_sketch_internal);
    *out = *sketch_ip;	/* struct copy */

    if (out->vert_count)
	out->verts = (point2d_t *)bu_calloc(out->vert_count, sizeof(point2d_t), "out->verts");

    for (i=0; sketch_ip->verts && i<out->vert_count; i++) {
	V2MOVE(out->verts[i], sketch_ip->verts[i]);
    }

    crv_out = &out->curve;
    if (crv_out)
	rt_copy_curve(crv_out, &sketch_ip->curve);

    return out;
}


static int
curve_to_tcl_list(struct bu_vls *vls, struct rt_curve *crv)
{
    size_t i, j;

    bu_vls_printf(vls, " SL {");
    for (j=0; j<crv->count; j++) {
	switch ((*(uint32_t *)crv->segment[j])) {
	    case CURVE_LSEG_MAGIC:
		{
		    struct line_seg *lsg = (struct line_seg *)crv->segment[j];
		    bu_vls_printf(vls, " { line S %d E %d }", lsg->start, lsg->end);
		}
		break;
	    case CURVE_CARC_MAGIC:
		{
		    struct carc_seg *csg = (struct carc_seg *)crv->segment[j];
		    bu_vls_printf(vls, " { carc S %d E %d R %.25g L %d O %d }",
				  csg->start, csg->end, csg->radius,
				  csg->center_is_left, csg->orientation);
		}
		break;
	    case CURVE_BEZIER_MAGIC:
		{
		    struct bezier_seg *bsg = (struct bezier_seg *)crv->segment[j];
		    bu_vls_printf(vls, " { bezier D %d P {", bsg->degree);
		    for (i=0; i<=(size_t)bsg->degree; i++)
			bu_vls_printf(vls, " %d", bsg->ctl_points[i]);
		    bu_vls_printf(vls, " } }");
		}
		break;
	    case CURVE_NURB_MAGIC:
		{
		    size_t k;
		    struct nurb_seg *nsg = (struct nurb_seg *)crv->segment[j];
		    bu_vls_printf(vls, " { nurb O %d T %d K {",
				  nsg->order, nsg->pt_type);
		    for (k=0; k<(size_t)nsg->k.k_size; k++)
			bu_vls_printf(vls, " %.25g", nsg->k.knots[k]);
		    bu_vls_strcat(vls, "} P {");
		    for (k=0; k<(size_t)nsg->c_size; k++)
			bu_vls_printf(vls, " %d", nsg->ctl_points[k]);
		    if (nsg->weights) {
			bu_vls_strcat(vls, "} W {");
			for (k=0; k<(size_t)nsg->c_size; k++)
			    bu_vls_printf(vls, " %.25g", nsg->weights[k]);
		    }
		    bu_vls_strcat(vls, "} }");
		}
		break;
	}
    }
    bu_vls_strcat(vls, " }");	/* end of segment list */

    return 0;
}


int rt_sketch_form(struct bu_vls *logstr, const struct rt_functab *ftp)
{
    BU_CK_VLS(logstr);
    RT_CK_FUNCTAB(ftp);

    bu_vls_printf(logstr, "V {%%f %%f %%f} A {%%f %%f %%f} B {%%f %%f %%f} VL {{%%f %%f} {%%f %%f} ...} SL {{segment_data} {segment_data}}");

    return BRLCAD_OK;
}


int
rt_sketch_get(struct bu_vls *logstr, const struct rt_db_internal *intern, const char *attr)
{
    struct rt_sketch_internal *skt=(struct rt_sketch_internal *)intern->idb_ptr;
    size_t i;
    struct rt_curve *crv;

    BU_CK_VLS(logstr);
    RT_SKETCH_CK_MAGIC(skt);

    if (attr == (char *)NULL) {
	bu_vls_strcpy(logstr, "sketch");
	bu_vls_printf(logstr, " V {%.25g %.25g %.25g}", V3ARGS(skt->V));
	bu_vls_printf(logstr, " A {%.25g %.25g %.25g}", V3ARGS(skt->u_vec));
	bu_vls_printf(logstr, " B {%.25g %.25g %.25g}", V3ARGS(skt->v_vec));
	bu_vls_strcat(logstr, " VL {");
	for (i=0; i<skt->vert_count; i++)
	    bu_vls_printf(logstr, " {%.25g %.25g}", V2ARGS(skt->verts[i]));
	bu_vls_strcat(logstr, " }");

	crv = &skt->curve;
	if (curve_to_tcl_list(logstr, crv)) {
	    return BRLCAD_ERROR;
	}
    } else if (BU_STR_EQUAL(attr, "V")) {
	bu_vls_printf(logstr, "%.25g %.25g %.25g", V3ARGS(skt->V));
    } else if (BU_STR_EQUAL(attr, "A")) {
	bu_vls_printf(logstr, "%.25g %.25g %.25g", V3ARGS(skt->u_vec));
    } else if (BU_STR_EQUAL(attr, "B")) {
	bu_vls_printf(logstr, "%.25g %.25g %.25g", V3ARGS(skt->v_vec));
    } else if (BU_STR_EQUAL(attr, "VL")) {
	for (i=0; i<skt->vert_count; i++)
	    bu_vls_printf(logstr, " {%.25g %.25g}", V2ARGS(skt->verts[i]));
    } else if (BU_STR_EQUAL(attr, "SL")) {
	crv = &skt->curve;
	if (curve_to_tcl_list(logstr, crv)) {
	    return BRLCAD_ERROR;
	}
    } else if (*attr == 'V') {
	long lval = atol((attr+1));
	if (lval < 0 || (size_t)lval >= skt->vert_count) {
	    bu_vls_printf(logstr, "ERROR: Illegal vertex number\n");
	    return BRLCAD_ERROR;
	}
	bu_vls_printf(logstr, "%.25g %.25g", V2ARGS(skt->verts[lval]));
    } else {
	/* unrecognized attribute */
	bu_vls_printf(logstr, "ERROR: Unknown attribute, choices are V, A, B, VL, SL, or V#\n");
	return BRLCAD_ERROR;
    }

    return BRLCAD_OK;
}


int
get_tcl_curve(struct bu_vls *logstr, struct rt_curve *crv, const char *argv1)
{
    int count;
    int j;
    const char **seg_list = NULL;

    /* split initial list */
    if (bu_argv_from_tcl_list(argv1, &count, (const char ***)&seg_list) != 0) {
	return -1;
    }

    if (count) {
	crv->count = count;
	crv->reverse = (int *)bu_calloc(count, sizeof(int), "crv->reverse");
	crv->segment = (void **)bu_calloc(count, sizeof(void *), "crv->segment");
    }

    /* loop through all the segments */
    for (j = 0; j < count; j++) {
	int seg_argc;
	const char **seg_argv = NULL;
	const char *elem, *sval;
	int k;

	/* get the next segment */
	if (bu_argv_from_tcl_list(seg_list[j], &seg_argc, (const char ***)&seg_argv) != 0) {
	    return -1;
	}

	if (seg_argc < 1) return 0;

	/* get the next segment */
	if (BU_STR_EQUAL(seg_argv[0], "line")) {
	    struct line_seg *lsg;

	    BU_ALLOC(lsg, struct line_seg);
	    for (k=1; k<seg_argc; k += 2) {
		elem = seg_argv[k];
		sval = seg_argv[k+1];
		switch (*elem) {
		    case 'S':
			(void)bu_opt_int(NULL, 1, &sval, (void *)&lsg->start);
			break;
		    case 'E':
			(void)bu_opt_int(NULL, 1, &sval, (void *)&lsg->end);
			break;
		}
	    }
	    lsg->magic = CURVE_LSEG_MAGIC;
	    crv->segment[j] = (void *)lsg;
	} else if (BU_STR_EQUAL(seg_argv[0], "bezier")) {
	    struct bezier_seg *bsg;
	    int num_points;

	    BU_ALLOC(bsg, struct bezier_seg);
	    for (k=1; k<seg_argc; k+= 2) {
		elem = seg_argv[k];
		sval = seg_argv[k+1];
		switch (*elem) {
		    case 'D': /* degree */
			(void)bu_opt_int(NULL, 1, &sval, (void *)&bsg->degree);
			break;
		    case 'P': /* list of control points */
			num_points = 0;
			(void)_rt_tcl_list_to_int_array(sval, &bsg->ctl_points, &num_points);
			if (num_points != bsg->degree + 1) {
			    bu_vls_printf(logstr, "ERROR: degree and number of control points disagree for a Bezier segment\n");
			    bu_free((char *)seg_argv, "free seg argv");
			    bu_free((char *)seg_list, "free seg argv");
			    return 1;
			}
		}
	    }
	    bsg->magic = CURVE_BEZIER_MAGIC;
	    crv->segment[j] = (void *)bsg;
	} else if (BU_STR_EQUAL(seg_argv[0], "carc")) {
	    struct carc_seg *csg;

	    BU_ALLOC(csg, struct carc_seg);
	    for (k=1; k<seg_argc; k += 2) {
		elem = seg_argv[k];
		sval = seg_argv[k+1];
		switch (*elem) {
		    case 'S':
			(void)bu_opt_int(NULL, 1, &sval, (void *)&csg->start);
			break;
		    case 'E':
			(void)bu_opt_int(NULL, 1, &sval, (void *)&csg->end);
			break;
		    case 'R':
			(void)bu_opt_fastf_t(NULL, 1, &sval, (void *)&csg->radius);
			break;
		    case 'L' :
			(void)bu_opt_bool(NULL, 1, &sval, (void *)&csg->center_is_left);
			break;
		    case 'O':
			(void)bu_opt_bool(NULL, 1, &sval, (void *)&csg->orientation);
			break;
		}
	    }
	    csg->magic = CURVE_CARC_MAGIC;
	    crv->segment[j] = (void *)csg;
	} else if (BU_STR_EQUAL(seg_argv[0], "nurb")) {
	    struct nurb_seg *nsg;

	    BU_ALLOC(nsg, struct nurb_seg);
	    for (k=1; k<seg_argc; k += 2) {
		elem = seg_argv[k];
		sval = seg_argv[k+1];
		switch (*elem) {
		    case 'O':
			(void)bu_opt_int(NULL, 1, &sval, (void *)&nsg->order);
			break;
		    case 'T':
			(void)bu_opt_int(NULL, 1, &sval, (void *)&nsg->pt_type);
			break;
		    case 'K':
			(void)_rt_tcl_list_to_fastf_array(sval, &nsg->k.knots, &nsg->k.k_size);
			break;
		    case 'P' :
			(void)_rt_tcl_list_to_int_array(sval, &nsg->ctl_points, &nsg->c_size);
			break;
		    case 'W':
			(void)_rt_tcl_list_to_fastf_array(sval, &nsg->weights, &nsg->c_size);
			break;
		}
	    }
	    nsg->magic = CURVE_NURB_MAGIC;
	    crv->segment[j] = (void *)nsg;
	} else {
	    bu_vls_sprintf(logstr, "ERROR: Unrecognized segment type: %s\n", seg_argv[0]);
	    bu_free((char *)seg_argv, "free seg argv");
	    bu_free((char *)seg_list, "free seg argv");
	    return 1;
	}

	bu_free((char *)seg_argv, "free seg argv");
    }

    bu_free((char *)seg_list, "free seg argv");

    return 0;
}


int
rt_sketch_adjust(struct bu_vls *logstr, struct rt_db_internal *intern, int argc, const char **argv)
{
    struct rt_sketch_internal *skt;
    int ret, array_len;
    fastf_t *newval;

    RT_CK_DB_INTERNAL(intern);
    skt = (struct rt_sketch_internal *)intern->idb_ptr;
    RT_SKETCH_CK_MAGIC(skt);

    while (argc >= 2) {
	if (BU_STR_EQUAL(argv[0], "V")) {
	    newval = skt->V;
	    array_len = 3;
	    if (_rt_tcl_list_to_fastf_array(argv[1], &newval, &array_len) !=
		array_len) {
		bu_vls_printf(logstr, "ERROR: Incorrect number of coordinates for vertex\n");
		return BRLCAD_ERROR;
	    }
	} else if (BU_STR_EQUAL(argv[0], "A")) {
	    newval = skt->u_vec;
	    array_len = 3;
	    if (_rt_tcl_list_to_fastf_array(argv[1], &newval, &array_len) !=
		array_len) {
		bu_vls_printf(logstr, "ERROR: Incorrect number of coordinates for vertex\n");
		return BRLCAD_ERROR;
	    }
	} else if (BU_STR_EQUAL(argv[0], "B")) {
	    newval = skt->v_vec;
	    array_len = 3;
	    if (_rt_tcl_list_to_fastf_array(argv[1], &newval, &array_len) !=
		array_len) {
		bu_vls_printf(logstr, "ERROR: Incorrect number of coordinates for vertex\n");
		return BRLCAD_ERROR;
	    }
	} else if (BU_STR_EQUAL(argv[0], "VL")) {
	    fastf_t *new_verts=(fastf_t *)NULL;
	    int len;
	    char *ptr;
	    char *dupstr;

	    /* the vertex list is a list of lists (each element is a
	     * list of two coordinates) so eliminate all the '{' and
	     * '}' chars in the list
	     */
	    dupstr = bu_strdup(argv[1]);

	    ptr = dupstr;
	    while (*ptr != '\0') {
		if (*ptr == '{' || *ptr == '}')
		    *ptr = ' ';
		ptr++;
	    }

	    len = 0;
	    (void)_rt_tcl_list_to_fastf_array(dupstr, &new_verts, &len);
	    bu_free(dupstr, "sketch adjust strdup");

	    if (len%2) {
		bu_vls_printf(logstr, "ERROR: Incorrect number of coordinates for vertices\n");
		return BRLCAD_ERROR;
	    }

	    if (skt->verts)
		bu_free((char *)skt->verts, "verts");
	    skt->verts = (point2d_t *)new_verts;
	    skt->vert_count = len / 2;
	} else if (BU_STR_EQUAL(argv[0], "SL")) {
	    /* the entire segment list */
	    struct rt_curve *crv;

	    crv = &skt->curve;
	    crv->count = 0;
	    crv->reverse = (int *)NULL;
	    crv->segment = (void **)NULL;

	    if ((ret=get_tcl_curve(logstr, crv, argv[1])) != 0)
		return ret;
	} else if (*argv[0] == 'V' && isdigit((int)*(argv[0]+1))) {
	    /* changing a specific vertex */
	    long vert_no;
	    fastf_t *new_vert;

	    vert_no = atol(argv[0] + 1);
	    new_vert = skt->verts[vert_no];
	    if (vert_no < 0 || (size_t)vert_no > skt->vert_count) {
		bu_vls_printf(logstr, "ERROR: Illegal vertex number\n");
		return BRLCAD_ERROR;
	    }
	    array_len = 2;
	    if (_rt_tcl_list_to_fastf_array(argv[1], &new_vert, &array_len) != array_len) {
		bu_vls_printf(logstr, "ERROR: Incorrect number of coordinates for vertex\n");
		return BRLCAD_ERROR;
	    }
	}

	argc -= 2;
	argv += 2;
    }

    return BRLCAD_OK;
}


int
rt_sketch_params(struct pc_pc_set *UNUSED(ps), const struct rt_db_internal *ip)
{
    if (ip) RT_CK_DB_INTERNAL(ip);

    return 0;			/* OK */
}


void
rt_curve_reverse_segment(uint32_t *lng)
{
    struct line_seg *lsg;
    struct carc_seg *csg;
    struct bezier_seg *bsg;
    int tmp, i;

    switch (*lng) {
	case CURVE_LSEG_MAGIC:
	    lsg = (struct line_seg *)lng;
	    tmp = lsg->start;
	    lsg->start = lsg->end;
	    lsg->end = tmp;
	    break;
	case CURVE_CARC_MAGIC:
	    csg = (struct carc_seg *)lng;
	    if (csg->radius < 0.0) {
		/* this is a full circle */
		csg->orientation = !csg->orientation; /* no real effect, but just for completeness */
	    } else {
		tmp = csg->start;
		csg->start = csg->end;
		csg->end = tmp;
		csg->center_is_left = !csg->center_is_left;
		csg->orientation = !csg->orientation;
	    }
	    break;
	case CURVE_BEZIER_MAGIC:
	    bsg = (struct bezier_seg *)lng;
	    for (i=0; i<bsg->degree/2; i++) {
		tmp = bsg->ctl_points[i];
		bsg->ctl_points[i] = bsg->ctl_points[bsg->degree-i];
		bsg->ctl_points[bsg->degree-i] = tmp;
	    }
	    break;
    }
}


void
rt_curve_order_segments(struct rt_curve *crv)
{
    int i, j, k;
    int count;
    int start1 = 0;
    int start2 = 0;
    int start3 = 0;
    int end1 = 0;
    int end2 = 0;
    int end3 = 0;

    count = crv->count;
    if (count < 2) {
	return;
    }

    for (j=1; j<count; j++) {
	int tmp_reverse;
	void *tmp_seg;
	int fixed=0;

	i = j - 1;

	get_indices(crv->segment[i], &start1, &end1);
	get_indices(crv->segment[j], &start2, &end2);

	if (end1 == start2)
	    continue;

	for (k=j+1; k<count; k++) {
	    get_indices(crv->segment[k], &start3, &end3);
	    if (start3 != end1)
		continue;

	    /* exchange j and k segments */
	    tmp_seg = crv->segment[j];
	    crv->segment[j] = crv->segment[k];
	    crv->segment[k] = tmp_seg;

	    tmp_reverse = crv->reverse[j];
	    crv->reverse[j] = crv->reverse[k];
	    crv->reverse[k] = tmp_reverse;
	    fixed = 1;
	    break;
	}

	if (fixed)
	    continue;

	/* try reversing a segment */
	for (k=j; k<count; k++) {
	    get_indices(crv->segment[k], &start3, &end3);
	    if (end3 != end1)
		continue;

	    rt_curve_reverse_segment((uint32_t *)crv->segment[k]);

	    if (k != j) {
		/* exchange j and k segments */
		tmp_seg = crv->segment[j];
		crv->segment[j] = crv->segment[k];
		crv->segment[k] = tmp_seg;

		tmp_reverse = crv->reverse[j];
		crv->reverse[j] = crv->reverse[k];
		crv->reverse[k] = tmp_reverse;
	    }
	    break;
	}
    }
}

const char *
rt_sketch_keypoint(point_t *pt, const char *keystr, const mat_t mat, const struct rt_db_internal *ip, const struct bn_tol *UNUSED(tol))
{
    if (!pt || !ip)
	return NULL;

    point_t mpt = VINIT_ZERO;
    struct rt_sketch_internal *sketch = (struct rt_sketch_internal *)ip->idb_ptr;
    RT_SKETCH_CK_MAGIC(sketch);

    static const char *default_keystr = "V";
    const char *k = (keystr) ? keystr : default_keystr;

    if (BU_STR_EQUAL(k, default_keystr)) {
	VMOVE(mpt, sketch->V);
	goto sketch_kpt_end;
    }

    // No keystr matches - failed
    return NULL;

sketch_kpt_end:

    MAT4X3PNT(*pt, mat, mpt);

    return k;
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
