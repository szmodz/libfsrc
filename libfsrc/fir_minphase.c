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
#include "fft.h"
#include "xblas.h"
#include "bits.h"
#include <math.h>
#include <string.h>
#include <assert.h>

#define FSRC_LN_3	1.09861228866810969139525
#define FSRC_ILN_3	0.910239226626837393614240

static size_t nextpow3(size_t N)
{
	size_t L = (size_t)(exp(ceil(log((double)N) * FSRC_ILN_3) * FSRC_LN_3) + .5);
	assert(L % 3 == 0);
	return L;
}

/*
	Turns an arbitrary filter into a minimum-phase version
	The resulting minimum phase filter is suboptimal for a given length
*/
fsrc_err fsrc_fir_minphase(size_t N, const double *h, double *g)
{
	size_t L = 64 * N; /* accuracy depends on the fft size */
	if(N & 1)
		L = nextpow2(L);
	else /* the nyquist frequency bin is always zero for even length filters */
		L = nextpow3(L); /* using an odd-length fft avoids the problem */

	size_t K = L / 2 + 1;

	double *x = 0;
	fsrc_dcomplex *X = 0;
	fsrc_dfft fft = 0;
	fsrc_dfft ifft = 0;

	fsrc_err err = FSRC_E_NOMEM;

	x = (double*)fsrc_alloc(L * sizeof(double));	
	if(!x) goto cleanup;
	X = (fsrc_dcomplex*)fsrc_alloc(K * sizeof(fsrc_dcomplex));
	if(!X) goto cleanup;

	err = fsrc_drcdft_init(&fft, L, x, X, 0);
	if(err != FSRC_S_OK) goto cleanup;
	err = fsrc_dcrdft_init(&ifft, L, X, x, 0);
	if(err != FSRC_S_OK) goto cleanup;

	memcpy(x, h, N * sizeof(double));
	memset(x + N, 0, (L - N) * sizeof(double));
	
	fsrc_drcdft(fft, x, X);

	for(size_t i = 0; i < K; ++i) {
		double A = X[i][0] * X[i][0] + X[i][1] * X[i][1];
		assert(A > 0);
		X[i][0] = .5 * log(A) / L;
		X[i][1] = 0;
	}

	fsrc_dcrdft(ifft, X, x);

	for(size_t i = 1; i < (L + 1) / 2; ++i)
		x[i] *= 2;

	memset(x + K, 0, (L - K) * sizeof(double));

	fsrc_drcdft(fft, x, X);

	for(size_t i = 0; i < K; ++i) {
		double ex = exp(X[i][0]) / L;
		X[i][0] = ex * cos(X[i][1]);
		X[i][1] = ex * sin(X[i][1]);
	}

	fsrc_dcrdft(ifft, X, x);

	memcpy(g, x, N * sizeof(double));

	err = FSRC_S_OK;
	
cleanup:
	if(fft) fsrc_dfft_destroy(fft);
	if(ifft) fsrc_dfft_destroy(ifft);

	fsrc_free(X);
	fsrc_free(x);

	return err;
}

/*
	Design of Optimal Minimum-Phase Digital FIR Filters Using Discrete Hilbert Transforms
	Niranjan Damera-Venkata, Brian L. Evans, and Shawn R. McCaslin	
*/

fsrc_err fsrc_fir_minphase_dht(size_t N, const double *h, double *g, double dp, double ds)
{
	size_t M = (N + 1) / 2;

	assert(N & 1);

	size_t L = nextpow2(64 * N);
	size_t K = L / 2 + 1;

	double s, a, scal;

	double *x = 0;
	double *A = 0;
	fsrc_dcomplex *X = 0;
	fsrc_dfft fft = 0;
	fsrc_dfft ifft = 0;

	fsrc_err err = FSRC_E_NOMEM;

	x = (double*)fsrc_alloc(L * sizeof(double));	
	if(!x) goto cleanup;
	A = (double*)fsrc_alloc(K * sizeof(double));	
	if(!A) goto cleanup;
	X = (fsrc_dcomplex*)fsrc_alloc(K * sizeof(fsrc_dcomplex));
	if(!X) goto cleanup;

	err = fsrc_drcdft_init(&fft, L, x, X, 0);
	if(err != FSRC_S_OK) goto cleanup;
	err = fsrc_dcrdft_init(&ifft, L, X, x, 0);
	if(err != FSRC_S_OK) goto cleanup;

	fsrc_dcopy(M, h, 1, x, -1);
	memcpy(x + L - N + M, h, (N - M) * sizeof(double));
	memset(x + M, 0, (L - N) * sizeof(double));

	fsrc_drcdft(fft, x, X);

	s = 1.0 / L;
	a = 1 + ds;
	scal = 2 / (a + sqrt((a + dp) * (a - dp)));

	for(size_t m = 0; m < K; ++m) {
		A[m] = sqrt((X[m][0] + ds) * scal);
		assert(A[m] > 0);

		X[m][0] = s * log(A[m]);
		X[m][1] = 0;
	}	

	fsrc_dcrdft(ifft, X, x);
	
	x[0] = 0;
	for(size_t m = 1; m < L / 2; ++m) {
		x[L - m] = -x[m];
	}
	x[L / 2] = 0;

	fsrc_drcdft(fft, x, X);

	for(size_t m = 0; m < K; ++m) {
		double a = s * A[m];	
		X[m][0] = a * cos(X[m][1]);
		X[m][1] = a * sin(X[m][1]);
	}

	fsrc_dcrdft(ifft, X, x);

	memcpy(g, x, M * sizeof(double));

cleanup:
	if(fft) fsrc_dfft_destroy(fft);
	if(ifft) fsrc_dfft_destroy(ifft);

	fsrc_free(X);
	fsrc_free(A);
	fsrc_free(x);

	return FSRC_S_OK;
}

