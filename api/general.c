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

#include <math.h>
#include <float.h>

#include "nlopt.h"
#include "nlopt-util.h"

/*************************************************************************/

int nlopt_isinf(double x) {
     return fabs(x) >= HUGE_VAL * 0.99
#ifdef HAVE_ISINF
	  || isinf(x)
#endif
	  ;
}
/*************************************************************************/

void nlopt_version(int *major, int *minor, int *bugfix)
{
     *major = MAJOR_VERSION;
     *minor = MINOR_VERSION;
     *bugfix = BUGFIX_VERSION;
}

/*************************************************************************/

static const char nlopt_algorithm_names[NLOPT_NUM_ALGORITHMS][256] = {
     "DIRECT (global, no-derivative)",
     "DIRECT-L (global, no-derivative)",
     "Randomized DIRECT-L (global, no-derivative)",
     "Unscaled DIRECT (global, no-derivative)",
     "Unscaled DIRECT-L (global, no-derivative)",
     "Unscaled Randomized DIRECT-L (global, no-derivative)",
     "Original DIRECT version (global, no-derivative)",
     "Original DIRECT-L version (global, no-derivative)",
#ifdef WITH_CXX
     "StoGO (global, derivative-based)",
     "StoGO with randomized search (global, derivative-based)",
#else
     "StoGO (NOT COMPILED)",
     "StoGO randomized (NOT COMPILED)",
#endif
#ifdef WITH_NOCEDAL_LBFGS
     "original NON-FREE L-BFGS code by Nocedal et al. (local, deriv.-based)",
#else
     "original NON-FREE L-BFGS code by Nocedal et al. (NOT COMPILED)",
#endif
     "Limited-memory BFGS (L-BFGS) (local, derivative-based)",
     "Principal-axis, praxis (local, no-derivative)",
     "Limited-memory variable-metric, rank 1 (local, derivative-based)",
     "Limited-memory variable-metric, rank 2 (local, derivative-based)",
     "Truncated Newton (local, derivative-based)",
     "Truncated Newton with restarting (local, derivative-based)",
     "Preconditioned truncated Newton (local, derivative-based)",
     "Preconditioned truncated Newton with restarting (local, derivative-based)",
     "Controlled random search (CRS2) with local mutation (global, no-derivative)",
     "Multi-level single-linkage (MLSL), random (global, no-derivative)",
     "Multi-level single-linkage (MLSL), random (global, derivative)",
     "Multi-level single-linkage (MLSL), quasi-random (global, no-derivative)",
     "Multi-level single-linkage (MLSL), quasi-random (global, derivative)",
     "Method of Moving Asymptotes (MMA) (local, derivative)",
     "COBYLA (Constrained Optimization BY Linear Approximations) (local, no-derivative)",
     "NEWUOA unconstrained optimization via quadratic models (local, no-derivative)",
     "Bound-constrained optimization via NEWUOA-based quadratic models (local, no-derivative)",
     "Nelder-Mead simplex algorithm (local, no-derivative)",
     "Sbplx variant of Nelder-Mead (re-implementation of Rowan's Subplex) (local, no-derivative)",
     "Augmented Lagrangian method (local, no-derivative)",
     "Augmented Lagrangian method (local, derivative)",
     "Augmented Lagrangian method for equality constraints (local, no-derivative)",
     "Augmented Lagrangian method for equality constraints (local, derivative)",
     "BOBYQA bound-constrained optimization via quadratic models (local, no-derivative)",
     "ISRES evolutionary constrained optimization (global, no-derivative)",
};

const char *nlopt_algorithm_name(nlopt_algorithm a)
{
     if (((int) a) < 0 || a >= NLOPT_NUM_ALGORITHMS) return "UNKNOWN";
     return nlopt_algorithm_names[a];
}

/*************************************************************************/

int nlopt_srand_called = 0;
void nlopt_srand(unsigned long seed) {
     nlopt_srand_called = 1;
     nlopt_init_genrand(seed);
}

void nlopt_srand_time() {
     nlopt_srand(nlopt_time_seed());
}

/*************************************************************************/