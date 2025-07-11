/*                  S A M P L E _ P N T S . C
 * BRL-CAD
 *
 * Copyright (c) 2015-2025 United States Government as represented by
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
/** @file sample_pnts.c
 *
 * Use the raytracer to sample points from an object
 *
 */
#include "common.h"
#include <time.h>
#include <string.h> /* for memset */

#include "vmath.h"
#include "bu/log.h"
#include "bu/ptbl.h"
#include "bn/mat.h"
#include "bn/sobol.h"
#include "bu/time.h"
#include "raytrace.h"

typedef int (*hitfunc_t)(struct application *, struct partition *, struct seg *);
typedef int (*missfunc_t)(struct application *);
typedef int (*overlapfunc_t)(struct application *, struct partition *, struct region *, struct region *, struct partition *);

struct rt_gen_worker_vars {
    struct rt_i *rtip;
    struct resource *resp;
    size_t rays_cnt;
    fastf_t *rays;
    hitfunc_t fhit;
    missfunc_t fmiss;
    overlapfunc_t foverlap;
    int step;       /* number of rays to be fired by this worker before calling back */
    int *ind_src;   /* source of starting index */
    int curr_ind;   /* current ray index */
    void *ptr; /* application specific info */
};

static int
rt_get_bbox_rays(fastf_t **rays, point_t min, point_t max, struct bn_tol *tol)
{
    int ret = 0;
    int count = 0;
    point_t mid;
    struct rt_pattern_data *xdata = NULL;
    struct rt_pattern_data *ydata = NULL;
    struct rt_pattern_data *zdata = NULL;

    if (!rays || !tol) return 0;

    if (NEAR_ZERO(DIST_PNT_PNT_SQ(min, max), VUNITIZE_TOL)) return 0;

    /* We've got the bounding box - set up the grids */
    VSET(mid, (max[0] + min[0])/2, (max[1] + min[1])/2, (max[2] + min[2])/2);

    BU_GET(xdata, struct rt_pattern_data);
    VSET(xdata->center_pt, min[0] - 0.1 * fabs(min[0]), mid[1], mid[2]);
    VSET(xdata->center_dir, 1, 0, 0);
    xdata->vn = 2;
    xdata->pn = 2;
    xdata->n_vec = (vect_t *)bu_calloc(xdata->vn + 1, sizeof(vect_t), "vects array");
    xdata->n_p = (fastf_t *)bu_calloc(xdata->pn + 1, sizeof(fastf_t), "params array");
    xdata->n_p[0] = tol->dist;
    xdata->n_p[1] = tol->dist;
    VSET(xdata->n_vec[0], 0, max[1] - mid[1], 0);
    VSET(xdata->n_vec[1], 0, 0, max[2] - mid[2]);
    ret = rt_pattern(xdata, RT_PATTERN_RECT_ORTHOGRID);
    bu_free(xdata->n_vec, "x vec inputs");
    bu_free(xdata->n_p, "x p inputs");
    if (ret < 0) {
	count = ret;
	goto memfree;
    }

    BU_GET(ydata, struct rt_pattern_data);
    VSET(ydata->center_pt, mid[0], min[1] - 0.1 * fabs(min[1]), mid[2]);
    VSET(ydata->center_dir, 0, 1, 0);
    ydata->vn = 2;
    ydata->pn = 2;
    ydata->n_vec = (vect_t *)bu_calloc(ydata->vn + 1, sizeof(vect_t), "vects array");
    ydata->n_p = (fastf_t *)bu_calloc(ydata->pn + 1, sizeof(fastf_t), "params array");
    ydata->n_p[0] = tol->dist;
    ydata->n_p[1] = tol->dist;
    VSET(ydata->n_vec[0], max[0] - mid[0], 0, 0);
    VSET(ydata->n_vec[1], 0, 0, max[2] - mid[2]);
    ret = rt_pattern(ydata, RT_PATTERN_RECT_ORTHOGRID);
    bu_free(ydata->n_vec, "y vec inputs");
    bu_free(ydata->n_p, "y p inputs");
    if (ret < 0) {
	count = ret;
	goto memfree;
    }

    BU_GET(zdata, struct rt_pattern_data);
    VSET(zdata->center_pt, mid[0], mid[1], min[2] - 0.1 * fabs(min[2]));
    VSET(zdata->center_dir, 0, 0, 1);
    zdata->vn = 2;
    zdata->pn = 2;
    zdata->n_vec = (vect_t *)bu_calloc(zdata->vn + 1, sizeof(vect_t), "vects array");
    zdata->n_p = (fastf_t *)bu_calloc(zdata->pn + 1, sizeof(fastf_t), "params array");
    zdata->n_p[0] = tol->dist;
    zdata->n_p[1] = tol->dist;
    VSET(zdata->n_vec[0], max[0] - mid[0], 0, 0);
    VSET(zdata->n_vec[1], 0, max[1] - mid[1], 0);
    ret = rt_pattern(zdata, RT_PATTERN_RECT_ORTHOGRID);
    bu_free(zdata->n_vec, "x vec inputs");
    bu_free(zdata->n_p, "x p inputs");
    if (ret < 0) {
	count = ret;
	goto memfree;
    }

    /* Consolidate the grids into a single ray array */
    {
	size_t si, sj;
	(*rays) = (fastf_t *)bu_calloc((xdata->ray_cnt + ydata->ray_cnt + zdata->ray_cnt + 1) * 6, sizeof(fastf_t), "rays");
	count = 0;
	for (si = 0; si < xdata->ray_cnt; si++) {
	    for (sj = 0; sj < 6; sj++) {
		(*rays)[6*count+sj] = xdata->rays[6*si + sj];
	    }
	    count++;
	}
	for (si = 0; si < ydata->ray_cnt; si++) {
	    for (sj = 0; sj < 6; sj++) {
		(*rays)[6*count+sj] = ydata->rays[6*si + sj];
	    }
	    count++;
	}
	for (si = 0; si < zdata->ray_cnt; si++) {
	    for (sj = 0; sj < 6; sj++) {
		(*rays)[6*count+sj] = zdata->rays[6*si+sj];
	    }
	    count++;
	}

    }

memfree:
    /* Free memory not stored in tables */
    if (xdata && xdata->rays) bu_free(xdata->rays, "x rays");
    if (ydata && ydata->rays) bu_free(ydata->rays, "y rays");
    if (zdata && zdata->rays) bu_free(zdata->rays, "z rays");
    if (xdata) BU_PUT(xdata, struct rt_pattern_data);
    if (ydata) BU_PUT(ydata, struct rt_pattern_data);
    if (zdata) BU_PUT(zdata, struct rt_pattern_data);
    return count;
}

static void
rt_gen_worker(int cpu, void *ptr)
{
    struct application ap;
    struct rt_gen_worker_vars *state = &(((struct rt_gen_worker_vars *)ptr)[cpu]);
    size_t start_ind, end_ind, i;
    int state_jmp = 0;

    RT_APPLICATION_INIT(&ap);
    ap.a_rt_i = state->rtip;
    ap.a_hit = state->fhit;
    ap.a_miss = state->fmiss;
    ap.a_overlap = state->foverlap;
    ap.a_onehit = 0;
    ap.a_logoverlap = rt_silent_logoverlap;
    ap.a_resource = state->resp;
    ap.a_uptr = (void *)state;

    /* Because a zero step means an infinite loop, ensure we are moving ahead
     * at least 1 ray each step */
    if (!state->step) state_jmp = 1;

    bu_semaphore_acquire(RT_SEM_WORKER);
    start_ind = (*state->ind_src);
    (*state->ind_src) = start_ind + state->step + state_jmp;
    //bu_log("increment(%d): %d\n", cpu, start_ind);
    bu_semaphore_release(RT_SEM_WORKER);
    end_ind = start_ind + state->step + state_jmp - 1;
    while (start_ind < state->rays_cnt) {
	for (i = start_ind; i <= end_ind && i < state->rays_cnt; i++) {
	    state->curr_ind = i;
	    VSET(ap.a_ray.r_pt, state->rays[6*i+0], state->rays[6*i+1], state->rays[6*i+2]);
	    VSET(ap.a_ray.r_dir, state->rays[6*i+3], state->rays[6*i+4], state->rays[6*i+5]);
	    rt_shootray(&ap);
	}
	bu_semaphore_acquire(RT_SEM_WORKER);
	start_ind = (*state->ind_src);
	(*state->ind_src) = start_ind + state->step + state_jmp;
	//bu_log("increment(%d): %d\n", cpu, start_ind);
	bu_semaphore_release(RT_SEM_WORKER);
	end_ind = start_ind + state->step + state_jmp - 1;
    }
}

struct pnt_normal_thickness {
    struct pnt_normal *pt;
    fastf_t thickness;
};

struct pnt_normal_thickness *
pnthickness_create() {
    struct pnt_normal_thickness *p;
    BU_GET(p, struct pnt_normal_thickness);
    BU_ALLOC(p->pt, struct pnt_normal);
    return p;
}

/* p->pt may be  used to construct the final pnts object */
void
pnthickness_free(struct pnt_normal_thickness *p, int free_pnt) {
    if (!p) return;
    if (free_pnt) {
	bu_free(p->pt, "free pnt");
    }
    BU_PUT(p, struct pnt_normal_thickness);
}

static void
_tgc_hack_fix(struct partition *part, struct soltab *stp) {
    /* hack fix for bad tgc surfaces - avoids a logging crash, which is probably something else altogether... */
    if (bu_strncmp("rec", stp->st_meth->ft_label, 3) == 0 || bu_strncmp("tgc", stp->st_meth->ft_label, 3) == 0) {

	/* correct invalid surface number */
	if (part->pt_inhit->hit_surfno < 1 || part->pt_inhit->hit_surfno > 3) {
	    part->pt_inhit->hit_surfno = 2;
	}
	if (part->pt_outhit->hit_surfno < 1 || part->pt_outhit->hit_surfno > 3) {
	    part->pt_outhit->hit_surfno = 2;
	}
    }
}

static int
outer_pnts_hit(struct application *ap, struct partition *PartHeadp, struct seg *UNUSED(segs))
{
    double thickness = 0.0;
    struct pnt_normal_thickness *in_pt, *out_pt;
    struct partition *in_part = PartHeadp->pt_forw;
    struct partition *out_part = PartHeadp->pt_back;
    struct soltab *stp = in_part->pt_inseg->seg_stp;
    struct soltab *ostp = out_part->pt_inseg->seg_stp;
    struct rt_gen_worker_vars *s = (struct rt_gen_worker_vars *)(ap->a_uptr);
    struct bu_ptbl *pnset = (struct bu_ptbl *)(s->ptr);

    RT_CK_APPLICATION(ap);

    _tgc_hack_fix(in_part, stp);
    _tgc_hack_fix(out_part, ostp);

    in_pt = pnthickness_create();
    VJOIN1(in_pt->pt->v, ap->a_ray.r_pt, in_part->pt_inhit->hit_dist, ap->a_ray.r_dir);
    RT_HIT_NORMAL(in_pt->pt->n, in_part->pt_inhit, stp, &(app->a_ray), in_part->pt_inflip);
    in_pt->thickness = 0.0;
    bu_ptbl_ins(pnset, (long *)in_pt);

    /* add "out" hit point info (unless half-space) */
    if (bu_strncmp("half", ostp->st_meth->ft_label, 4) != 0) {
	out_pt = pnthickness_create();
	VJOIN1(out_pt->pt->v, ap->a_ray.r_pt, out_part->pt_outhit->hit_dist, ap->a_ray.r_dir);
	RT_HIT_NORMAL(out_pt->pt->n, out_part->pt_outhit, ostp, &(app->a_ray), out_part->pt_outflip);
	// If we don't have a viable normal, we don't want the point
	if (!VNEAR_ZERO(out_pt->pt->n, VUNITIZE_TOL)) {
	    bu_ptbl_ins(pnset, (long *)out_pt);
	    thickness = DIST_PNT_PNT(in_pt->pt->v, out_pt->pt->v) * 0.5;
	    in_pt->thickness = thickness;
	    out_pt->thickness = thickness;
	} else {
	    pnthickness_free(out_pt, 1);
	}
    }

    return 0;
}

static int
all_pnts_hit(struct application *app, struct partition *partH, struct seg *UNUSED(segs))
{
    double thickness = 0.0;
    struct pnt_normal_thickness *in_pt, *out_pt;
    struct partition *pp;
    struct soltab *stp;
    struct rt_gen_worker_vars *s = (struct rt_gen_worker_vars *)(app->a_uptr);
    struct bu_ptbl *pnset = (struct bu_ptbl *)(s->ptr);

    RT_CK_APPLICATION(app);

    /* add all hit points */
    for (pp = partH->pt_forw; pp != partH; pp = pp->pt_forw) {

	/* always add in hit */
	stp = pp->pt_inseg->seg_stp;
	_tgc_hack_fix(pp, stp);
	in_pt = pnthickness_create();
	VJOIN1(in_pt->pt->v, app->a_ray.r_pt, pp->pt_inhit->hit_dist, app->a_ray.r_dir);
	RT_HIT_NORMAL(in_pt->pt->n, pp->pt_inhit, stp, &(app->a_ray), pp->pt_inflip);
	in_pt->thickness = 0.0;
	// If we don't have a viable normal, we don't want the point
	if (!VNEAR_ZERO(in_pt->pt->n, VUNITIZE_TOL)) {
	    bu_ptbl_ins(pnset, (long *)in_pt);
	} else {
	    pnthickness_free(in_pt, 1);
	    continue;
	}

	/* add "out" hit point unless it's a half-space */
	if (bu_strncmp("half", stp->st_meth->ft_label, 4) != 0) {
	    out_pt = pnthickness_create();
	    VJOIN1(out_pt->pt->v, app->a_ray.r_pt, pp->pt_outhit->hit_dist, app->a_ray.r_dir);
	    RT_HIT_NORMAL(out_pt->pt->n, pp->pt_outhit, stp, &(app->a_ray), pp->pt_outflip);
	    // If we don't have a viable normal, we don't want the point
	    if (!VNEAR_ZERO(out_pt->pt->n, VUNITIZE_TOL)) {
		bu_ptbl_ins(pnset, (long *)out_pt);
	    } else {
		pnthickness_free(out_pt, 1);
		continue;
	    }
	    thickness = DIST_PNT_PNT(in_pt->pt->v, out_pt->pt->v) * 0.5;
	    in_pt->thickness = thickness;
	    out_pt->thickness = thickness;
	}
    }

    return 0;
}

static int
op_overlap(struct application *ap, struct partition *UNUSED(pp),
		struct region *UNUSED(reg1), struct region *UNUSED(reg2),
		struct partition *UNUSED(hp))
{
    RT_CK_APPLICATION(ap);
    return 0;
}


static int
op_miss(struct application *ap)
{
    RT_CK_APPLICATION(ap);
    return 0;
}

static void
prand_pnt_worker(int cpu, void *ptr)
{
    struct application ap;
    struct rt_gen_worker_vars *state = &(((struct rt_gen_worker_vars *)ptr)[cpu]);
    size_t i;

    RT_APPLICATION_INIT(&ap);
    ap.a_rt_i = state->rtip;
    ap.a_hit = state->fhit;
    ap.a_miss = state->fmiss;
    ap.a_overlap = state->foverlap;
    ap.a_onehit = 0;
    ap.a_logoverlap = rt_silent_logoverlap;
    ap.a_resource = state->resp;
    ap.a_uptr = (void *)state;

    for (i = 0; i < state->rays_cnt; i++) {
	VSET(ap.a_ray.r_pt, state->rays[6*i+0], state->rays[6*i+1], state->rays[6*i+2]);
	VSET(ap.a_ray.r_dir, state->rays[6*i+3], state->rays[6*i+4], state->rays[6*i+5]);
	rt_shootray(&ap);
    }
}

void
get_random_rays(fastf_t *rays, long int craynum, point_t center, fastf_t radius)
{
    long int i = 0;
    point_t p1, p2;
    if (!rays) return;
    for (i = 0; i < craynum; i++) {
	vect_t n;
	bn_rand_sph_sample(p1, center, radius);
	bn_rand_sph_sample(p2, center, radius);
	VSUB2(n, p2, p1);
	VUNITIZE(n);
	rays[i*6+0] = p1[X];
	rays[i*6+1] = p1[Y];
	rays[i*6+2] = p1[Z];
	rays[i*6+3] = n[X];
	rays[i*6+4] = n[Y];
	rays[i*6+5] = n[Z];
    }
}

void
get_sobol_rays(fastf_t *rays, long int craynum, point_t center, fastf_t radius, struct bn_soboldata *s)
{
    long int i = 0;
    point_t p1;
    if (!rays) return;
    for (i = 0; i < craynum; i++) {
	vect_t n;
	bn_sobol_sph_sample(p1, center, radius, s);
	VSUB2(n, center, p1);
	VUNITIZE(n);
	rays[i*6+0] = p1[X];
	rays[i*6+1] = p1[Y];
	rays[i*6+2] = p1[Z];
	rays[i*6+3] = n[X];
	rays[i*6+4] = n[Y];
	rays[i*6+5] = n[Z];
    }
}

/* 0 = success, -1 error */
int
rt_gen_obj_pnts(struct rt_pnts_internal *rpnts, fastf_t *avg_thickness, struct db_i *dbip,
       const char *obj, struct bn_tol *tol, int flags, int max_pnts, int max_time, int verbosity)
{
    int pntcnt = 0;
    size_t i;
    int ret, j;
    int do_grid = 1;
    fastf_t oldtime, currtime;
    int ind = 0;
    int count = 0;
    double avgt = 0.0;
    struct rt_i *rtip = NULL;
    size_t ncpus = bu_avail_cpus();
    struct rt_gen_worker_vars *state = (struct rt_gen_worker_vars *)bu_calloc(ncpus+1, sizeof(struct rt_gen_worker_vars ), "state");
    struct bu_ptbl **grid_pnts = NULL;
    struct bu_ptbl **rand_pnts = NULL;
    struct bu_ptbl **sobol_pnts = NULL;
    struct resource *resp = (struct resource *)bu_calloc(ncpus+1, sizeof(struct resource), "resources");
    int pntcnt_grid = 0;
    int pntcnt_rand = 0;
    int pntcnt_sobol = 0;

    if (!dbip || !obj || !tol || ncpus == 0) {
	ret = 0;
	goto memfree;
    }

    /* Only skip the grid if it's 1) not explicitly on and 2) we have another method */
    if (!(flags & RT_GEN_OBJ_PNTS_GRID)) {
	if ((flags & RT_GEN_OBJ_PNTS_RAND) || (flags & RT_GEN_OBJ_PNTS_SOBOL)) {
	    do_grid = 0;
	}
    }

    oldtime = bu_gettime();

    rtip = rt_new_rti(dbip);

    for (i = 0; i < ncpus+1; i++) {
	/* standard */
	state[i].rtip = rtip;
	if (flags & RT_GEN_OBJ_PNTS_SURF) {
	    state[i].fhit = outer_pnts_hit;
	} else {
	    state[i].fhit = all_pnts_hit;
	}
	state[i].fmiss = op_miss;
	state[i].foverlap = op_overlap;
	state[i].resp = &resp[i];
	state[i].ind_src = &ind;
	rt_init_resource(state[i].resp, (int)i, rtip);
    }
    if (rt_gettree(rtip, obj) < 0) return -1;

    rt_prep_parallel(rtip, (int)ncpus);

    currtime = bu_gettime();

    if (verbosity > 1) {
	bu_log("Object to Point Set prep time: %.1f\n", (currtime - oldtime)/1.0e6);
    }

    /* Regardless of whether or not we're shooting the grid, it is our
     * guide for how many rays to fire */
    if (do_grid) {
	fastf_t *rays = NULL;
	grid_pnts = (struct bu_ptbl **)bu_calloc(ncpus+1, sizeof(struct bu_ptbl *), "local state");
	for (i = 0; i < ncpus+1; i++) {
	    /* per-cpu hit point storage */
	    BU_GET(grid_pnts[i], struct bu_ptbl);
	    bu_ptbl_init(grid_pnts[i], 64, "first and last hit points");
	    state[i].ptr = (void *)grid_pnts[i];
	}
	count = rt_get_bbox_rays(&rays, rtip->mdl_min, rtip->mdl_max, tol);
	for (i = 0; i < ncpus+1; i++) {
	    state[i].step = (int)(count/ncpus * 0.1);
	    state[i].rays_cnt = count;
	    state[i].rays = rays;
	}
	oldtime = bu_gettime();
	bu_parallel(rt_gen_worker, ncpus, (void *)state);
	currtime = bu_gettime();
	for (i = 0; i < ncpus+1; i++) {
	    state[i].rays = NULL;
	}
	if (verbosity > 1) {
	    bu_log("analyze time: %.1f\n", (currtime - oldtime)/1.0e6);
	}
    }

    if (flags & RT_GEN_OBJ_PNTS_RAND || flags & RT_GEN_OBJ_PNTS_SOBOL) {
	/* The formula 16*r^2/t^2 is based on the point at which the sum of the
	 * area of the circles around each point with radius 0.5*t equals the
	 * surface area of the bounding sphere, and thus adjusting the
	 * tolerance scales the number of rays as a function of the object
	 * size. However, before this can go live a way is needed to make sure
	 * it doesn't set the maximum to crazy large levels... maybe capping it
	 * at some ratio of bsph radius/tol */
	/* long int mrc = (max_ray_cnt) ? max_ray_cnt : (long int)((16 * rtip->rti_radius*rtip->rti_radius)/tol->dist_sq); */

	/* We now know enough to get the max ray count.  Try up to 1.5x the number of max
	 * points of rays, or up to 2 million if no max was specified. */
	size_t mrc = (!max_pnts) ? 2000000 : max_pnts * 1.5;
	size_t ccnt = (ncpus >= LONG_MAX-1) ? ncpus : ncpus+1;
	size_t craynum = mrc/ccnt;
	fastf_t mt = (max_time > 0) ? (fastf_t)max_time : (fastf_t)INT_MAX;

	point_t center;

	VADD2SCALE(center, rtip->mdl_max, rtip->mdl_min, 0.5);

	if (flags & RT_GEN_OBJ_PNTS_RAND) {
	    fastf_t delta = 0;
	    size_t raycnt = 0;
	    int pc = 0;
	    rand_pnts = (struct bu_ptbl **)bu_calloc(ncpus+1, sizeof(struct bu_ptbl *), "local state");
	    for (i = 0; i < ncpus+1; i++) {
		/* per-cpu hit point storage */
		BU_GET(rand_pnts[i], struct bu_ptbl);
		bu_ptbl_init(rand_pnts[i], 64, "first and last hit points");
		state[i].ptr = (void *)rand_pnts[i];
		if (!state[i].rays) {
		    state[i].rays = (fastf_t *)bu_calloc(craynum * 6 + 1, sizeof(fastf_t), "rays");
		    state[i].rays_cnt = craynum;
		}
	    }
	    oldtime = bu_gettime();
	    while (delta < mt && raycnt < mrc && (!max_pnts || pc < max_pnts)) {
		for (i = 0; i < ncpus+1; i++) {
		    get_random_rays(state[i].rays, craynum, center, rtip->rti_radius);
		}
		bu_parallel(prand_pnt_worker, ncpus, (void *)state);
		raycnt += craynum * (ncpus+1);
		pc = 0;
		for (i = 0; i < ncpus+1; i++) {
		    pc += (int)BU_PTBL_LEN(rand_pnts[i]);
		}
		if (max_pnts && (pc >= max_pnts)) break;
		delta = (bu_gettime() - oldtime)/1e6;
	    }
	}

	if (flags & RT_GEN_OBJ_PNTS_SOBOL) {
	    fastf_t delta = 0;
	    size_t raycnt = 0;
	    int pc = 0;
	    struct bn_soboldata *sobolseq = bn_sobol_create(2, time(NULL));
	    bn_sobol_skip(sobolseq, (int)craynum);
	    oldtime = bu_gettime();
	    sobol_pnts = (struct bu_ptbl **)bu_calloc(ncpus+1, sizeof(struct bu_ptbl *), "local state");
	    for (i = 0; i < ncpus+1; i++) {
		/* per-cpu hit point storage */
		BU_GET(sobol_pnts[i], struct bu_ptbl);
		bu_ptbl_init(sobol_pnts[i], 64, "first and last hit points");
		state[i].ptr = (void *)sobol_pnts[i];
		if (!state[i].rays) {
		    state[i].rays = (fastf_t *)bu_calloc(craynum * 6 + 1, sizeof(fastf_t), "rays");
		    state[i].rays_cnt = craynum;
		}
	    }
	    while (delta < mt && raycnt < mrc && (!max_pnts || pc < max_pnts)) {
		for (i = 0; i < ncpus+1; i++) {
		    get_sobol_rays(state[i].rays, craynum, center, rtip->rti_radius, sobolseq);
		}
		bu_parallel(prand_pnt_worker, ncpus, (void *)state);
		raycnt += craynum * (ncpus+1);
		pc = 0;
		for (i = 0; i < ncpus+1; i++) {
		    pc += (int)BU_PTBL_LEN(sobol_pnts[i]);
		}
		if (max_pnts && (pc >= max_pnts)) break;
		delta = (bu_gettime() - oldtime)/1e6;
	    }
	    bn_sobol_destroy(sobolseq);
	}
    }

    /* Collect and print all of the results */

    for (i = 0; i < ncpus+1; i++) {
	if (grid_pnts) {
	    pntcnt_grid += (int)BU_PTBL_LEN(grid_pnts[i]);
	}
	if (rand_pnts) {
	    pntcnt_rand += (int)BU_PTBL_LEN(rand_pnts[i]);
	}
	if (sobol_pnts) {
	    pntcnt_sobol += (int)BU_PTBL_LEN(sobol_pnts[i]);
	}
    }
    /* we probably don't want to cap grid points, but cap the others */
    if (max_pnts && pntcnt_rand > max_pnts) pntcnt_rand = max_pnts;
    if (max_pnts && pntcnt_sobol > max_pnts) pntcnt_sobol = max_pnts;

    pntcnt = pntcnt_grid + pntcnt_rand + pntcnt_sobol;
    if (pntcnt < 1) {
	ret = BRLCAD_ERROR;
    } else {
	int pc = 0;
	int total_pnts = 0;
	long double thickness_total = 0.0;
	if (rpnts) {
	    rpnts->count = pntcnt;
	    BU_ALLOC(rpnts->point, struct pnt_normal);
	    BU_LIST_INIT(&(((struct pnt_normal *)rpnts->point)->l));
	}
	/* Grid points */
	if (grid_pnts) {
	    pc = 0;
	    for (i = 0; i < ncpus+1; i++) {
		for (j = 0; j < (int)BU_PTBL_LEN(grid_pnts[i]); j++) {
		    struct pnt_normal_thickness *pnthick = (struct pnt_normal_thickness *)BU_PTBL_GET(grid_pnts[i], j);
		    if (pc < pntcnt_grid) {
			if (rpnts) {
			    BU_LIST_PUSH(&(((struct pnt_normal *)rpnts->point)->l), &(pnthick->pt)->l);
			}
			thickness_total += pnthick->thickness;
			pnthickness_free(pnthick, 0);
			total_pnts++;
		    } else {
			pnthickness_free(pnthick, 1);
		    }
		    pc++;
		}
	    }
	}
	/* Random points */
	if (rand_pnts) {
	    pc = 0;
	    for (i = 0; i < ncpus+1; i++) {
		for (j = 0; j < (int)BU_PTBL_LEN(rand_pnts[i]); j++) {
		    struct pnt_normal_thickness *pnthick = (struct pnt_normal_thickness *)BU_PTBL_GET(rand_pnts[i], j);
		    if (pc < pntcnt_rand) {
			if (rpnts) {
			    BU_LIST_PUSH(&(((struct pnt_normal *)rpnts->point)->l), &(pnthick->pt)->l);
			}
			thickness_total += pnthick->thickness;
			pnthickness_free(pnthick, 0);
			total_pnts++;
		    } else {
			pnthickness_free(pnthick, 1);
		    }
		    pc++;
		}
	    }
	}
	/* Sobol points */
	if (sobol_pnts) {
	    pc = 0;
	    for (i = 0; i < ncpus+1; i++) {
		for (j = 0; j < (int)BU_PTBL_LEN(sobol_pnts[i]); j++) {
		    struct pnt_normal_thickness *pnthick = (struct pnt_normal_thickness *)BU_PTBL_GET(sobol_pnts[i], j);
		    if (pc < pntcnt_sobol) {
			if (rpnts) {
			    BU_LIST_PUSH(&(((struct pnt_normal *)rpnts->point)->l), &(pnthick->pt)->l);
			}
			thickness_total += pnthick->thickness;
			pnthickness_free(pnthick, 0);
			total_pnts++;
		    } else {
			pnthickness_free(pnthick, 1);
		    }
		    pc++;
		}
	    }
	}
	ret = 0;

	avgt = thickness_total / (double)total_pnts;
	if (avg_thickness) (*avg_thickness) = avgt;
    }


memfree:
    /* Free memory not stored in tables */
    for (i = 0; i < ncpus+1; i++) {
	if (grid_pnts) {
	    if (grid_pnts[i] != NULL) {
		bu_ptbl_free(grid_pnts[i]);
		BU_PUT(grid_pnts[i], struct bu_ptbl);
	    }
	}
	if (rand_pnts) {
	    if (rand_pnts[i] != NULL) {
		bu_ptbl_free(rand_pnts[i]);
		BU_PUT(rand_pnts[i], struct bu_ptbl);
	    }
	}
	if (sobol_pnts) {
	    if (sobol_pnts[i] != NULL) {
		bu_ptbl_free(sobol_pnts[i]);
		BU_PUT(sobol_pnts[i], struct bu_ptbl);
	    }
	}
    }
    for (i = 0; i < ncpus+1; i++) {
	/* free rays */
	if (state[i].rays) {
	    bu_free(state[i].rays, "rand rays");
	}
    }
    if (grid_pnts) bu_free(grid_pnts, "free state containers");
    if (rand_pnts) bu_free(rand_pnts, "free state containers");
    if (sobol_pnts) bu_free(sobol_pnts, "free state containers");

    for (i = 0; i < ncpus+1; i++) {
	rt_clean_resource(rtip, &resp[i]);
    }
    rt_free_rti(rtip);
    bu_free(state, "free state containers");
    bu_free(resp, "free resources array");
    return ret;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

