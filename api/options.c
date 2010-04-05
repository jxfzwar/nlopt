/* Copyright (c) 2007-2010 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "nlopt-internal.h"
#include "nlopt-util.h"

/*************************************************************************/

void nlopt_destroy(nlopt_opt opt)
{
     if (opt) {
	  free(opt->lb); free(opt->ub);
	  free(opt->xtol_abs);
	  free(opt->fc);
	  free(opt->h);
	  nlopt_destroy(opt->local_opt);
	  free(opt->dx);
	  free(opt);
     }
}

nlopt_opt nlopt_create(nlopt_algorithm algorithm, unsigned n)
{
     nlopt_opt opt;

     if (((int) algorithm) < 0 || algorithm >= NLOPT_NUM_ALGORITHMS)
	  return NULL;

     opt = (nlopt_opt) malloc(sizeof(struct nlopt_opt_s));
     if (opt) {
	  opt->algorithm = algorithm;
	  opt->n = n;
	  opt->f = NULL; opt->f_data = NULL;
	  opt->maximize = 0;

	  opt->lb = opt->ub = NULL;
	  opt->m = opt->m_alloc = 0;
	  opt->fc = NULL;
	  opt->p = opt->p_alloc = 0;
	  opt->h = NULL;

	  opt->stopval = -HUGE_VAL;
	  opt->ftol_rel = opt->ftol_abs = 0;
	  opt->xtol_rel = 0; opt->xtol_abs = NULL;
	  opt->maxeval = 0;
	  opt->maxtime = 0;

	  opt->local_opt = NULL;
	  opt->stochastic_population = 0;
	  opt->dx = NULL;

	  if (n > 0) {
	       opt->lb = (double *) malloc(sizeof(double) * (n));
	       if (!opt->lb) goto oom;
	       opt->ub = (double *) malloc(sizeof(double) * (n));
	       if (!opt->ub) goto oom;
	       opt->xtol_abs = (double *) malloc(sizeof(double) * (n));
	       if (!opt->xtol_abs) goto oom;
	       nlopt_set_lower_bounds1(opt, -HUGE_VAL);
	       nlopt_set_upper_bounds1(opt, +HUGE_VAL);
	       nlopt_set_xtol_abs1(opt, 0.0);
	  }
     }

     return opt;

oom:
     nlopt_destroy(opt);
     return NULL;
}

nlopt_opt nlopt_copy(const nlopt_opt opt)
{
     nlopt_opt nopt = NULL;
     if (opt) {
	  nopt = (nlopt_opt) malloc(sizeof(struct nlopt_opt_s));
	  *nopt = *opt;
	  nopt->lb = nopt->ub = nopt->xtol_abs = NULL;
	  nopt->fc = nopt->h = NULL;
	  nopt->m_alloc = nopt->p_alloc = 0;
	  nopt->local_opt = NULL;
	  nopt->dx = NULL;

	  if (opt->n > 0) {
	       nopt->lb = (double *) malloc(sizeof(double) * (opt->n));
	       if (!opt->lb) goto oom;
	       nopt->ub = (double *) malloc(sizeof(double) * (opt->n));
	       if (!opt->ub) goto oom;
	       nopt->xtol_abs = (double *) malloc(sizeof(double) * (opt->n));
	       if (!opt->xtol_abs) goto oom;
	       
	       memcpy(nopt->lb, opt->lb, sizeof(double) * (opt->n));
	       memcpy(nopt->ub, opt->ub, sizeof(double) * (opt->n));
	       memcpy(nopt->xtol_abs, opt->xtol_abs, sizeof(double) * (opt->n));
	  }

	  if (opt->m) {
	       nopt->m_alloc = opt->m;
	       nopt->fc = (nlopt_constraint *) malloc(sizeof(nlopt_constraint)
						      * (opt->m));
	       if (!nopt->fc) goto oom;
	       memcpy(nopt->fc, opt->fc, sizeof(nlopt_constraint) * (opt->m));
	  }

	  if (opt->p) {
	       nopt->p_alloc = opt->p;
	       nopt->h = (nlopt_constraint *) malloc(sizeof(nlopt_constraint)
						     * (opt->p));
	       if (!nopt->h) goto oom;
	       memcpy(nopt->h, opt->h, sizeof(nlopt_constraint) * (opt->p));
	  }

	  if (opt->local_opt) {
	       nopt->local_opt = nlopt_copy(opt->local_opt);
	       if (!nopt->local_opt) goto oom;
	  }

	  if (opt->dx) {
	       nopt->dx = (double *) malloc(sizeof(double) * (opt->n));
	       if (!nopt->dx) goto oom;
	       memcpy(nopt->dx, opt->dx, sizeof(double) * (opt->n));
	  }
     }
     return nopt;

oom:
     nlopt_destroy(nopt);
     return NULL;
}

/*************************************************************************/

nlopt_result nlopt_set_min_objective(nlopt_opt opt, nlopt_func f, void *f_data)
{
     if (opt && f) {
	  opt->f = f; opt->f_data = f_data;
	  opt->maximize = 0;
	  if (nlopt_isinf(opt->stopval) && opt->stopval > 0)
	       opt->stopval = -HUGE_VAL;
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_set_max_objective(nlopt_opt opt, nlopt_func f, void *f_data)
{
     if (opt && f) {
	  opt->f = f; opt->f_data = f_data;
	  opt->maximize = 1;
	  if (nlopt_isinf(opt->stopval) && opt->stopval < 0)
	       opt->stopval = +HUGE_VAL;
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

/*************************************************************************/

nlopt_result nlopt_set_lower_bounds(nlopt_opt opt, const double *lb)
{
     if (opt && (opt->n == 0 || lb)) {
	  memcpy(opt->lb, lb, sizeof(double) * (opt->n));
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_set_lower_bounds1(nlopt_opt opt, double lb)
{
     if (opt) {
	  unsigned i;
	  for (i = 0; i < opt->n; ++i)
	       opt->lb[i] = lb;
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_get_lower_bounds(nlopt_opt opt, double *lb)
{
     if (opt && (opt->n == 0 || lb)) {
	  memcpy(lb, opt->lb, sizeof(double) * (opt->n));
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_set_upper_bounds(nlopt_opt opt, const double *ub)
{
     if (opt && (opt->n == 0 || ub)) {
	  memcpy(opt->ub, ub, sizeof(double) * (opt->n));
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_set_upper_bounds1(nlopt_opt opt, double ub)
{
     if (opt) {
	  unsigned i;
	  for (i = 0; i < opt->n; ++i)
	       opt->ub[i] = ub;
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_get_upper_bounds(nlopt_opt opt, double *ub)
{
     if (opt && (opt->n == 0 || ub)) {
	  memcpy(ub, opt->ub, sizeof(double) * (opt->n));
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

/*************************************************************************/

#define AUGLAG_ALG(a) ((a) == NLOPT_LN_AUGLAG ||        \
		       (a) == NLOPT_LN_AUGLAG_EQ ||     \
		       (a) == NLOPT_LD_AUGLAG ||        \
		       (a) == NLOPT_LD_AUGLAG_EQ)

nlopt_result nlopt_remove_inequality_constraints(nlopt_opt opt)
{
     free(opt->fc);
     opt->fc = NULL;
     opt->m = opt->m_alloc = 0;
     return NLOPT_SUCCESS;
}

static nlopt_result add_constraint(unsigned *m, unsigned *m_alloc,
				   nlopt_constraint **c,
				   nlopt_func fc, void *fc_data,
				   double tol)
{
     *m += 1;
     if (*m > *m_alloc) {
	  /* allocate by repeated doubling so that 
	     we end up with O(log m) mallocs rather than O(m). */
	  *m_alloc = 2 * (*m);
	  *c = (nlopt_constraint *) realloc(*c,
					    sizeof(nlopt_constraint)
					    * (*m_alloc));
	  if (!*c) {
	       *m_alloc = *m = 0;
	       return NLOPT_OUT_OF_MEMORY;
	  }
     }
     
     (*c)[*m - 1].f = fc;
     (*c)[*m - 1].f_data = fc_data;
     (*c)[*m - 1].tol = tol;
     return NLOPT_SUCCESS;
}

nlopt_result nlopt_add_inequality_constraint(nlopt_opt opt,
					     nlopt_func fc, void *fc_data,
					     double tol)
{
     if (opt && fc && tol >= 0) {
	  /* nonlinear constraints are only supported with some algorithms */
	  if (opt->algorithm != NLOPT_LD_MMA 
	      && opt->algorithm != NLOPT_LN_COBYLA
	      && !AUGLAG_ALG(opt->algorithm) 
	      && opt->algorithm != NLOPT_GN_ISRES)
	       return NLOPT_INVALID_ARGS;

	  return add_constraint(&opt->m, &opt->m_alloc, &opt->fc,
				fc, fc_data, tol);
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_remove_equality_constraints(nlopt_opt opt)
{
     free(opt->h);
     opt->h = NULL;
     opt->p = opt->p_alloc = 0;
     return NLOPT_SUCCESS;
}

nlopt_result nlopt_add_equality_constraint(nlopt_opt opt,
					   nlopt_func h, void *h_data,
					   double tol)
{
     if (opt && h && tol >= 0) {
	  /* equality constraints (h(x) = 0) only via some algorithms */
	  if (!AUGLAG_ALG(opt->algorithm) && opt->algorithm != NLOPT_GN_ISRES)
	       return NLOPT_INVALID_ARGS;

	  return add_constraint(&opt->p, &opt->p_alloc, &opt->h,
				h, h_data, tol);
     }
     return NLOPT_INVALID_ARGS;
}

/*************************************************************************/

#define SET(param, T, arg)				\
   nlopt_result nlopt_set_##param(nlopt_opt opt, T arg)	\
   {							\
	if (opt) {					\
	     opt->arg = arg;				\
	     return NLOPT_SUCCESS;			\
	}						\
	return NLOPT_INVALID_ARGS;			\
   }


#define GET(param, T, arg) T nlopt_get_##param(const nlopt_opt opt) {	\
        return opt->arg;						\
   }

#define GETSET(param, T, arg) GET(param, T, arg) SET(param, T, arg)

GETSET(stopval, double, stopval)

GETSET(ftol_rel, double, ftol_rel)
GETSET(ftol_abs, double, ftol_abs)
GETSET(xtol_rel, double, xtol_rel)

nlopt_result nlopt_set_xtol_abs(nlopt_opt opt, const double *xtol_abs)
{
     if (opt) {
	  memcpy(opt->xtol_abs, xtol_abs, opt->n & sizeof(double));
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_set_xtol_abs1(nlopt_opt opt, const double xtol_abs)
{
     if (opt) {
	  unsigned i;
	  for (i = 0; i < opt->n; ++i)
	       opt->xtol_abs[i] = xtol_abs;
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

nlopt_result nlopt_get_xtol_abs(const nlopt_opt opt, double *xtol_abs)
{
     memcpy(xtol_abs, opt->xtol_abs, opt->n & sizeof(double));
     return NLOPT_SUCCESS;
}

GETSET(maxeval, int, maxeval)

GETSET(maxtime, double, maxtime)

/*************************************************************************/

GET(algorithm, nlopt_algorithm, algorithm)
GET(dimension, unsigned, n)

/*************************************************************************/

nlopt_result nlopt_set_local_optimizer(nlopt_opt opt,
				       const nlopt_opt local_opt)
{
     if (opt) {
	  if (local_opt && local_opt->n != opt->n) return NLOPT_INVALID_ARGS;
	  nlopt_destroy(opt->local_opt);
	  opt->local_opt = nlopt_copy(local_opt);
	  if (local_opt) {
	       if (!opt->local_opt) return NLOPT_OUT_OF_MEMORY;
	       nlopt_set_lower_bounds(opt->local_opt, opt->lb);
	       nlopt_set_upper_bounds(opt->local_opt, opt->ub);
	       nlopt_remove_inequality_constraints(opt->local_opt);
	       nlopt_remove_equality_constraints(opt->local_opt);
	  }
	  return NLOPT_SUCCESS;
     }
     return NLOPT_INVALID_ARGS;
}

/*************************************************************************/

GETSET(population, unsigned, stochastic_population)

/*************************************************************************/

nlopt_result nlopt_set_initial_step1(nlopt_opt opt, double dx)
{
     unsigned i;
     if (!opt || dx == 0) return NLOPT_INVALID_ARGS;
     if (!opt->dx && opt->n > 0) {
	  opt->dx = (double *) malloc(sizeof(double) * (opt->n));
	  if (!opt->dx) return NLOPT_OUT_OF_MEMORY;
     }
     for (i = 0; i < opt->n; ++i) opt->dx[i] = dx;
     return NLOPT_SUCCESS;
}

nlopt_result nlopt_set_initial_step(nlopt_opt opt, const double *dx)
{
     unsigned i;
     if (!opt || !dx) return NLOPT_INVALID_ARGS;
     for (i = 0; i < opt->n; ++i) if (dx[i] == 0) return NLOPT_INVALID_ARGS;
     if (!opt->dx && nlopt_set_initial_step1(opt, 1) == NLOPT_OUT_OF_MEMORY)
          return NLOPT_OUT_OF_MEMORY;
     memcpy(opt->dx, dx, sizeof(double) * (opt->n));
     return NLOPT_SUCCESS;
}

nlopt_result nlopt_get_initial_step(const nlopt_opt opt, const double *x, 
				    double *dx)
{
     if (!opt) return NLOPT_INVALID_ARGS;
     if (!opt->n) return NLOPT_SUCCESS;
     if (!opt->dx) {
	  nlopt_opt o = (nlopt_opt) opt; /* discard const temporarily */
	  nlopt_result ret = nlopt_set_default_initial_step(o, x);
	  if (ret != NLOPT_SUCCESS) return ret;
	  memcpy(dx, o->dx, sizeof(double) * (opt->n));
	  free(o->dx); o->dx = NULL; /* don't save, since x-dependent */
     }
     else
	  memcpy(dx, opt->dx, sizeof(double) * (opt->n));
     return NLOPT_SUCCESS;
}

nlopt_result nlopt_set_default_initial_step(nlopt_opt opt, const double *x)
{
     const double *lb, *ub;
     unsigned i;

     if (!opt || !x) return NLOPT_INVALID_ARGS;
     lb = opt->lb; ub = opt->ub;

     if (!opt->dx && nlopt_set_initial_step1(opt, 1) == NLOPT_OUT_OF_MEMORY)
	  return NLOPT_OUT_OF_MEMORY;

     /* crude heuristics for initial step size of nonderivative algorithms */
     for (i = 0; i < opt->n; ++i) {
	  double step = HUGE_VAL;

	  if (!nlopt_isinf(ub[i]) && !nlopt_isinf(lb[i])
	      && (ub[i] - lb[i]) * 0.25 < step && ub[i] > lb[i])
	       step = (ub[i] - lb[i]) * 0.25;
	  if (!nlopt_isinf(ub[i]) 
	      && ub[i] - x[i] < step && ub[i] > x[i])
	       step = (ub[i] - x[i]) * 0.75;
	  if (!nlopt_isinf(lb[i]) 
	      && x[i] - lb[i] < step && x[i] > lb[i])
	       step = (x[i] - lb[i]) * 0.75;

	  if (nlopt_isinf(step)) {
	       if (!nlopt_isinf(ub[i]) 
		   && fabs(ub[i] - x[i]) < fabs(step))
		    step = (ub[i] - x[i]) * 1.1;
	       if (!nlopt_isinf(lb[i]) 
		   && fabs(x[i] - lb[i]) < fabs(step))
		    step = (x[i] - lb[i]) * 1.1;
	  }
	  if (nlopt_isinf(step) || step == 0) {
	       step = x[i];
	  }
	  if (nlopt_isinf(step) || step == 0)
	       step = 1;
	  
	  opt->dx[i] = step;
     }
     return NLOPT_SUCCESS;
}

/*************************************************************************/