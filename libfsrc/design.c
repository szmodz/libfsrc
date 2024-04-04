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
#include "design.h"
#include "rational.h"
#include "enum_factors.h"
#include <assert.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>

size_t ifsrc_cache_get_lpfs(fsrc_cache *cache, const fsrc_lps *lps, fsrc_lpc *lpc, size_t n);
fsrc_err ifsrc_cache_lpfs(fsrc_cache *cache, const fsrc_lps *lps, fsrc_lpc *lpc, size_t n);

typedef struct fsrc_mstage {
	fsrc_ratio r;
	double fp;
	double fs;
} fsrc_mstage;

typedef struct fsrc_mdata {
	size_t n;
	fsrc_mstage s[FSRC_MAX_STAGES];
} fsrc_mdata;

typedef struct fsrc_level {
	double cost;
	fsrc_rational *factors;
} fsrc_level;

typedef struct fsrc_factors {
	fsrc_rational r;
	double bw;
	fsrc_level *lev;
} fsrc_factors;

static unsigned gcd(unsigned a, unsigned b)
{
	unsigned t;
	while(b) {
		t = a % b;
		a = b;
		b = t;
	}
	return a;
}

static double cost_func(const fsrc_rational *q, size_t len, double bw, unsigned up, unsigned dn)
{
	assert(up < dn);

	double Fp = up * (1 + bw);

	unsigned Fi = dn;

	double c = 0;
	for(size_t i = 0; i < len; ++i) {
		unsigned ui = q[i].den;
		unsigned di = q[i].num;

		unsigned Fo = Fi / di * ui;
		assert(Fo >= up);

		c += (Fi * Fo) / (2 * MIN(Fi, Fo) - Fp); /* computation rate */
		/*c += (Fi * ui) / (2 * MIN(Fi, Fo) - Fp);*/ /* filter length */

		Fi = Fo;
	}

	assert(Fi == up);

	return c;
}

static void eval_factors(fsrc_factors *of, const fsrc_rational *r, size_t len)
{
	double c = cost_func(r, len, of->bw, of->r.den, of->r.num);
	fsrc_level *fl = &of->lev[len - 1];
	if(c < fl->cost) {
		fl->cost = c;	
		memcpy(fl->factors, r, len * sizeof(fsrc_rational));
	}
}

static fsrc_err fsrc_decompose(fsrc_mdata *ms, fsrc_ratio r, double pbw, int flags)
{
	fsrc_rational ff[(FSRC_MAX_STAGES * (FSRC_MAX_STAGES + 1)) / 2];
	fsrc_level lev[FSRC_MAX_STAGES];

	fsrc_rational *pr = ff;
	for(size_t i = 0; i < FSRC_MAX_STAGES; ++i) {
		lev[i].factors = pr;
		lev[i].cost = DBL_MAX;
		pr += i + 1;
	}

	fsrc_factors f;	
	if(r.up > r.dn) {
		f.r.num = r.up;
		f.r.den = r.dn;
	} else {
		f.r.num = r.dn;
		f.r.den = r.up;
	}
	f.bw = pbw;
	f.lev = lev;

	lev[0].factors[0] = f.r;
	lev[0].cost = cost_func(&f.r, 1, pbw, f.r.den, f.r.num);

	int nf = fsrc_enum_factors(FSRC_MAX_STAGES, f.r, (enum_proc_t)eval_factors, &f);
	
	/* just choose the two stage design for now */
	//int n = 1;
	//int n = (nf < 2) ? 1 : 2;
	int n = (nf < 3) ? 1 : 3;

	fsrc_rational *sr = lev[n - 1].factors;

	assert(f.r.den < f.r.num);

	double Fs = f.r.den;
	double Fp = Fs * pbw;

	unsigned Fi = f.r.num;

	for(int i = 0; i < n; ++i) {
		unsigned ui = sr[i].den;
		unsigned di = sr[i].num;

		ms->s[i].r.up = ui;
		ms->s[i].r.dn = di;

		unsigned Fo = Fi / di * ui;
		assert(Fo >= Fs);

		double Fhi = Fi * ui;

		ms->s[i].fs = (2 * MIN(Fi, Fo) - Fs) / Fhi;
		ms->s[i].fp = Fp / Fhi;

		Fi = Fo;
	}

	ms->n = n;

	assert(Fi == f.r.den);

	return FSRC_S_OK;
}

fsrc_err ifsrc_design(fsrc_cache *des, fsrc_spec *spec, fsrc_model *design)
{
	fsrc_ratio r = spec->fr;

	if(r.up == 0 || r.dn == 0)
		return FSRC_E_INVARG;

	unsigned div = gcd(r.up, r.dn);

	r.up /= div;
	r.dn /= div;

	unsigned up = r.up;
	unsigned dn = r.dn;

	if(up == 1 && dn == 1)
		return FSRC_E_INVARG;

	size_t size = MIN((spec->isize + dn - 1) / dn, (spec->osize + up - 1) / up);
	if(size == 0)
		return FSRC_E_INVARG;

	/*if(spec->flags & FSRC_FFT_SRC) {
		size = fsrc_dfft_nearest_low(size, FSRC_FFT_SIZE_ANY);
		if(size == 0)
			return FSRC_E_INVARG;
	}*/

	spec->isize = size * dn;
	spec->osize = size * up;

	fsrc_mdata ms;
	fsrc_err ret = fsrc_decompose(&ms, r, spec->bw, 0);
	if(ret != FSRC_S_OK)
		return ret;

	memset(design, 0, sizeof(fsrc_model));

	design->ratio = r;

	fsrc_stage_model *s = design->stages;

	fsrc_lps lps[FSRC_MAX_STAGES];
	fsrc_lpc lpc[FSRC_MAX_STAGES];

	memset(lps, 0, sizeof(lps));
	memset(lpc, 0, sizeof(lpc));

	double dp = spec->dp / ms.n;
	double ds = spec->ds;
	int flags = spec->flags & FSRC_LPF_MINPHASE;

	for(size_t i = 0; i < ms.n; ++i) {
		lps[i].fp = ms.s[i].fp;
		lps[i].fs = ms.s[i].fs;
		lps[i].dp = dp;
		lps[i].ds = ds;
		lps[i].flags = flags;		
	}

	if(ifsrc_cache_get_lpfs(des, lps, lpc, ms.n) < ms.n) {
		for(size_t i = 0; i < ms.n; ++i) {
			if(lpc[i].h == 0) {
				fsrc_err err = fsrc_lpf_design(&lpc[i], &lps[i]);
				if(err != FSRC_S_OK) {
					for(size_t j = 0; j < ms.n; ++j) {
						free(lpc[i].h);
					}
					return err;
				}
			}
		}
		ifsrc_cache_lpfs(des, lps, lpc, ms.n);
	}	
	
	if(up < dn) {
		for(size_t i = 0; i < ms.n; ++i) {
			s[i].ratio = ms.s[i].r;
			s[i].h = lpc[i].h;
			s[i].n = lpc[i].n;
		}
	} else {
		for(size_t i = 0; i < ms.n; ++i) {
			size_t j = ms.n - i - 1;
			s[j].ratio.up = ms.s[i].r.dn;
			s[j].ratio.dn = ms.s[i].r.up;
			s[j].h = lpc[i].h;
			s[j].n = lpc[i].n;
		}
	}

	/* fill in buffer sizes */

	fsrc_bufsize *bs = design->sizes;

	size_t osize = size * dn;
	for(size_t i = 0; i < ms.n; ++i) {
		size_t Li = s[i].ratio.up;
		size_t Mi = s[i].ratio.dn;

		/* hack: implement negative positions? */
		/* Mi > ms[i].n shouldn't happen unless the desired quality is quite bad. */
		/* in which case you probably should use a different of resampling anyway. */
		size_t past = (MAX(s[i].n, Mi) + Li - 1) / Li - 1;

		bs[i].past = past;
		bs[i].size = osize + past;

		osize = osize / Mi * Li;
	}

	bs[ms.n].past = 0;
	bs[ms.n].size = osize;

	design->nstages = ms.n;

	return FSRC_S_OK;
}

fsrc_err fsrc_cache_design(fsrc_cache *cache, fsrc_spec *spec)
{
	fsrc_model design;
	if(cache == 0)
		return FSRC_E_INVARG;

	fsrc_err err = ifsrc_design(cache, spec, &design);
	if(err == FSRC_S_OK) {
		ifsrc_model_free(&design);
	}

	return err;
}

void ifsrc_model_free(fsrc_model *model)
{
	for(size_t i = 0; i < model->nstages; ++i) {
		fsrc_free(model->stages[i].h);
	}
}


