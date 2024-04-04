/*    
	Copyright (C) 2009 Szymon Modzelewski

	This file is part of libfsrc.

    libfsrc is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libfsrc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libfsrc.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "ifsrc.h"
#include "lpf_design.h"
#include "fir_irls.h"
#include "fft.h"
#include "xblas.h"
#include <math.h>
#include <string.h>
#include <assert.h>

size_t lpf_len(double df, double d1, double d2)
{
	double logd1 = log10(d1);
	double logd2 = log10(d2);
	double logd12 = logd1 * logd1;

	double D =
		(5.309e-3 * logd12 + 7.114e-2 * logd1 - 4.761e-1) * logd2
		- 2.66e-3 * logd12 - 5.941e-1 * logd1 - 4.278e-1;

	return (size_t)ceil(2 * D / df + 1);
}

fsrc_err fsrc_fir_minphase_dht(size_t n, const double *h, double *g, double dp, double ds);
fsrc_err fsrc_fir_minphase(size_t N, const double *h, double *g);

#define FSRC_MINPHASE_OPT 0

fsrc_err fsrc_lpf_design(fsrc_lpc *lpf, const fsrc_lps *spec)
{
	fsrc_err err = FSRC_S_OK;

	double rp = spec->dp;
	double rs = spec->ds;
	int flags = spec->flags;

#if FSRC_MINPHASE_OPT
	if(flags & FSRC_LPF_MINPHASE) {
		double rden = 2 + 2 * rp * rp - rs * rs;
		rp = 4 * rp / rden;
		rs = rs * rs / rden;
	}
#endif

	double f[] = { spec->fp, spec->fs };
	double a[] = { 1, 0 };
	double delm = MIN(rp, rs);
	double w[] = { delm / rp, delm / rs };

	double del = delm;

	fir_irls_spec irls = {
		0, 0,
		2, f, a, w,
		1e-5, 1e-13
	};
#if 0
	if(del < 1e-8) {
		/* this is hackish. fix later. */
		irls.irls_tol = 5e-6;
		irls.pcg_tol = 1e-12;
	}
#endif

	double df = spec->fs - spec->fp;

	size_t n = lpf_len(df, rp, rs);

	for(;;) {
#if FSRC_MINPHASE_OPT
		if(flags & FSRC_LPF_MINPHASE)
			n |= 1;
#endif

		irls.h = (double*)fsrc_alloc(n * sizeof(double));
		irls.n = n;

		fir_irls_info inf;
		int ret = fir_irls(&irls, &inf);
		if(ret <= 0) {
			fsrc_free(irls.h);
			return ret < 0 ? FSRC_E_INTERNAL : FSRC_E_NOMEM;
		}

		if(inf.del <= delm)
			break;

		del *= pow(delm / inf.del, 2);
		size_t m = lpf_len(df, del / w[0], del / w[1]);
		assert(m >= n);
		if(m == n) {
			n += 2;
		} else {
			n = m;
		}				
	}	

	size_t l = (n + 1) / 2;
	double *h = irls.h;
	
	memmove(h + n - l, h, l * sizeof(double));
	fsrc_dcopy(n - l, h + l, 1, h, -1);

	if(flags & FSRC_LPF_MINPHASE) {
#if FSRC_MINPHASE_OPT
		err = fsrc_fir_minphase_dht(n, h, h, rp, rs);
		if(err == FSRC_S_OK)
			n = (n + 1) / 2;
		else
			fsrc_free(h);
#else
		err = fsrc_fir_minphase(n, h, h);
		if(err != FSRC_S_OK)
			fsrc_free(h);
#endif
	}

	lpf->h = h;
	lpf->n = n;

	return err;
}
