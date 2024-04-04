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
#include "fir_irls.h"
#include "toeplitz_pcg.h"
#include "fft.h"
#include "bits.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "xblas.h"

#define IRLS_GRID_DENS 12
#define IRLS_MAX_ITER 200
#define IRLS_MAX_PCG_ITER 1000
#define IRLS_ALPHA 1.25

int fir_irls(const fir_irls_spec *spec, fir_irls_info *inf)
{
	fsrc_err err;
	toep_pcg *pcg;
	fsrc_dfft dct1, dct, idct;
	double irls_tol, pcg_tol;
	double *h, *W, *D, *A, *H, *G, *g, *ev, *gev, *kern;/*, wmax;*/
	const double *f, *a, *w;	
	size_t n, m, gev_size, ev_size, *n0 = 0;

	n = spec->n;
	h = spec->h;
	m = spec->m;
	f = spec->f;
	a = spec->a;
	w = spec->w;
	irls_tol = spec->irls_tol;
	pcg_tol = spec->pcg_tol;

	int opt = (n & 1) ? PCG_TOEP_SYM_WSHS : PCG_TOEP_SYM_HSHS;

	int ni = 0; /* iteration count */
	unsigned cgi = 0; /* pcg iterations */

	size_t l = (n + 1) / 2; /* number of unique coefficients */
	size_t grid_len = nextpow2(IRLS_GRID_DENS * n) + 1;
	double s = 1.0 / (2 * (grid_len - 1)); /* fft scale */

	pcg = 0;
	dct1 = dct = idct = 0;
	W = D = A = H = G = g = ev = gev = kern = 0;

	/* L2 weights */
	W = (double*)fsrc_alloc(grid_len * sizeof(double));
	if(!W) goto cleanup;
	/* Chebyshev weights */
	D = (double*)fsrc_alloc(grid_len * sizeof(double));
	if(!D) goto cleanup;
	/* Desired response */
	A = (double*)fsrc_alloc(grid_len * sizeof(double));
	if(!A) goto cleanup;
	/* G, H: general purpose buffers */
	H = (double*)fsrc_alloc(grid_len * sizeof(double));
	if(!H) goto cleanup;
	G = (double*)fsrc_alloc(grid_len * sizeof(double));
	if(!G) goto cleanup;

	/* previous coefficients */
	g = (double*)fsrc_alloc(n * sizeof(double));
	if(!g) goto cleanup;

	 /* band edge indices */
	n0 = (size_t*)malloc(2 * m * sizeof(size_t));
	if(!n0) goto cleanup;

	pcg = toep_pcg_init(n, opt);
	if(!n0) goto cleanup;

	gev_size = toep_pcg_circulant_size(pcg);
	ev_size = toep_pcg_precond_size(pcg);

	gev = (double*)fsrc_alloc(gev_size * sizeof(double));
	if(!gev) goto cleanup;
	ev = (double*)fsrc_alloc(ev_size * sizeof(double));
	if(!ev) goto cleanup;

	kern = toep_pcg_jackson(pcg, 3);
	if(!kern) goto cleanup;
	
	err = fsrc_ddtt_init(&dct1, grid_len, A, H, FSRC_DCT_1, 0);
	if(err != FSRC_S_OK) goto cleanup;
	if(n & 1) {
		dct = dct1;
		idct = dct1;
	} else {
		err = fsrc_ddtt_init(&dct, grid_len - 1, A, H, FSRC_DCT_2, 0);
		if(err != FSRC_S_OK) goto cleanup;
		err = fsrc_ddtt_init(&idct, grid_len - 1, H, A, FSRC_DCT_3, 0);
		if(err != FSRC_S_OK) goto cleanup;
	}

	/* compute band edge indices */
	n0[0] = 0;
	for (size_t i = 1; i < 2 * m - 1; i += 2)
		n0[i] = (size_t)ceil(f[i - 1] * (grid_len - 1));
	for (size_t i = 2; i < 2 * m - 1; i += 2)
		n0[i] = (size_t)floor(f[i - 1] * (grid_len - 1));
	n0[2 * m - 1] = grid_len;

	memset(W, 0, grid_len * sizeof(double));
	memset(D, 0, grid_len * sizeof(double));
	memset(A, 0, grid_len * sizeof(double));

	/*wmax = w[cblas_idamax(m, w, 1)];*/
	for(size_t i = 0; i < m; ++i) {
		size_t nl = n0[2 * i];
		size_t nr = n0[2 * i + 1];
		double wi = w[i]; /*/ wmax;*/
		double w2 = wi * wi;
		for(size_t j = nl; j < nr; ++j) {
			D[j] = wi;
			W[j] = w2;
			A[j] = a[i];
		}
	}

	memset(h, 0, n * sizeof(double));
	for(;;) {
		memcpy(g, h, n * sizeof(double));

		/* normalize weights */
		double WL2 = fsrc_dnrm2(grid_len, W);
		fsrc_dscal(grid_len, 1 / WL2, W);

		/* compute the first row of the Toeplitz system matrix */
		memcpy(H, W, grid_len * sizeof(double));
		fsrc_dscal(grid_len, s, H);
		fsrc_ddtt(dct1, H, G);

		toep_pcg_circulant_ev(pcg, G, gev);
		toep_pcg_jackson_ev(pcg, kern, G, ev);

		/* compute RHS */
		for(size_t j = 0; j < grid_len; ++j)
			H[j] = W[j] * A[j] * s;

		fsrc_ddtt(idct, H, G);	
		fsrc_dcopy(n - l, G + 2 * l - n, 1, G + l, -1);

		++ni;

		unsigned ncg = toep_pcg_solve(pcg, gev, ev, G, h, pcg_tol, IRLS_MAX_PCG_ITER);
		cgi += ncg;
		if(ncg == IRLS_MAX_PCG_ITER) {
			ni = -ni;
			break;
		}

		/* compute the frequency response */
		memcpy(G, h, l * sizeof(double));
		memset(G + l, 0, (grid_len - l) * sizeof(double));
		fsrc_ddtt(dct, G, H);

		/* check stop condition */
		fsrc_dxmy(n, h, g);
		if(fsrc_dnrm2(n, g) / fsrc_dnrm2(n, h) < irls_tol)
			break;

		if(ni == IRLS_MAX_ITER) {
			ni = -ni;
			break;
		}

		/* compute the error envelope */
		for(size_t j = 0; j < grid_len; ++j)
			H[j] = pow(D[j] * fabs(H[j] - A[j]), IRLS_ALPHA);

		for(size_t i = 0; i < m; ++i) {
			size_t nl = n0[2 * i];
			size_t nr = n0[2 * i + 1];

			while (nl < nr - 1) {
				// find the next peak
				size_t j = nl + 1;
				while(j < nr - 1 && (H[j - 1] > H[j] || H[j] < H[j + 1]))
					++j;
				// perform linear interpolation
				double a = (H[j] - H[nl]) / (j - nl);
				double c = H[nl] - a * nl;
				for(size_t k = nl; k < j; ++k) {
					double y = a * k + c;
					H[k] = MAX(H[k], y);
				}
				nl = j;
			}			
		}

		/* update weights */
		for(size_t j = 0; j < grid_len; ++j)
			W[j] *= H[j];
	}

	double del = 0;
	if(ni > 0) {
		for(size_t j = 0; j < grid_len; ++j) {
			double e = D[j] * fabs(H[j] - A[j]);
			if(e > del)
				del = e;
		}
	}

	inf->niter = ni;
	inf->npcg = cgi;
	inf->del = del;

cleanup:

	if(!(n & 1)) {
		if(dct) fsrc_dfft_destroy(dct);
		if(idct) fsrc_dfft_destroy(idct);
	}

	if(dct1) fsrc_dfft_destroy(dct1);
	if(pcg) toep_pcg_destroy(pcg);

	free(n0);

	fsrc_free(kern);
	fsrc_free(ev);
	fsrc_free(gev);
	fsrc_free(g);
	fsrc_free(G);
	fsrc_free(H);
	fsrc_free(A);
	fsrc_free(D);
	fsrc_free(W);

	return ni;
}

