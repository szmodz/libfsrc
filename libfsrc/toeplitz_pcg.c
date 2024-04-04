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
#include "toeplitz_pcg.h"
#include <stdlib.h>
#include <assert.h>
#include "fft.h"
#include "xblas.h"
#include "bits.h"
#include <string.h>

struct toep_pcg {
	size_t N; /* toeplitz size */
	size_t M; /* fft size */

	fsrc_dfft dct1; /* DCT-I of size M/2+1 */

	fsrc_dfft dct2; /* DCT-I of size M+1 */

	size_t M1;
	fsrc_dfft dft1; /* forward r2c DFT of size M or DCT-II of size M/2 or DCT-I of size M/2+1 */
	fsrc_dfft idft1; /* inverse c2r DFT of size M or DCT-III of size M/2 or DCT-I of size M/2+1 */

	size_t M2;
	fsrc_dfft dft2; /* forward r2c DFT of size 2M or DCT-II of size M or DCT-I of size M+1 */
	fsrc_dfft idft2; /* inverse c2r DFT of size 2M or DCT-III of size M or DCT-I of size M+1 */

	double *y;
	double *Y;
	double *r;
	double *d;

	int s; /* symmetry */
};

#define PCG_INIT_CHECK(a) if(!(a)) { toep_pcg_destroy(pcg); return 0; } else (void)0

toep_pcg *toep_pcg_init(size_t N, int opt)
{
	fsrc_err err;
	assert(N);

	int sym = (opt & PCG_TOEP_SYM_MASK);
	switch(sym)
	{
	case PCG_TOEP_SYM_NONE:
	case PCG_TOEP_SYM_WSHS:
	case PCG_TOEP_SYM_HSHS:
		break;
	default:
		return 0;
	}

	toep_pcg *pcg = (toep_pcg*)malloc(sizeof(toep_pcg));
	if(!pcg)
		return 0;

	memset(pcg, 0, sizeof(toep_pcg));

	size_t M = nextpow2(N);

	pcg->N = N;
	pcg->M = M;
	pcg->s = sym;

	int sflags = 0;
	if(opt & PCG_TOEP_OPT_SOLVE)
		sflags = FSRC_FFT_OPTIMIZE;

	if(!pcg->s) {
		size_t M2 = 2 * M;
		size_t K2 = M + 1;

		double *y = (double*)fsrc_alloc(M2 * sizeof(double));
		PCG_INIT_CHECK(y);
		pcg->y = y;

		fsrc_dcomplex *Y = (fsrc_dcomplex*)fsrc_alloc(K2 * sizeof(fsrc_dcomplex));
		PCG_INIT_CHECK(Y);
		pcg->Y = Y[0];

		pcg->M1 = M;
		pcg->M2 = M2;

		err = fsrc_drcdft_init(&pcg->dft1, M, y, Y, sflags);
		PCG_INIT_CHECK(err == FSRC_S_OK);
		err = fsrc_dcrdft_init(&pcg->idft1, M, Y, y, sflags);
		PCG_INIT_CHECK(err == FSRC_S_OK);

		err = fsrc_drcdft_init(&pcg->dft2, M2, y, Y, sflags);
		PCG_INIT_CHECK(err == FSRC_S_OK);
		err = fsrc_dcrdft_init(&pcg->idft2, M2, Y, y, sflags);
		PCG_INIT_CHECK(err == FSRC_S_OK);

	} else {
		double *y = (double*)fsrc_alloc((M + 1) * sizeof(double));
		PCG_INIT_CHECK(y);
		pcg->y = y;

		double *Y = (double*)fsrc_alloc((M + 1) * sizeof(double));
		PCG_INIT_CHECK(Y);
		pcg->Y = Y;
		
		if (sym == PCG_TOEP_SYM_HSHS) {
			size_t M1 = M / 2;

			pcg->M1 = M1;
			pcg->M2 = M;

			err = fsrc_ddtt_init(&pcg->dft1, M1, y, Y, FSRC_DCT_2, sflags);
			PCG_INIT_CHECK(err == FSRC_S_OK);
			err = fsrc_ddtt_init(&pcg->idft1, M1, Y, y, FSRC_DCT_3, sflags);
			PCG_INIT_CHECK(err == FSRC_S_OK);

			err = fsrc_ddtt_init(&pcg->dft2, M, y, Y, FSRC_DCT_2, sflags);
			PCG_INIT_CHECK(err == FSRC_S_OK);
			err = fsrc_ddtt_init(&pcg->idft2, M, Y, y, FSRC_DCT_3, sflags);	
			PCG_INIT_CHECK(err == FSRC_S_OK);
		}
	}

	sflags = 0;
	if((opt & PCG_TOEP_OPT_EIGEN) || (sym == PCG_TOEP_SYM_WSHS && (opt & PCG_TOEP_OPT_SOLVE)))
		sflags = FSRC_FFT_OPTIMIZE;

	size_t L1 = M / 2 + 1;
	size_t L2 = M + 1;

	err = fsrc_ddtt_init(&pcg->dct1, L1, pcg->y, pcg->Y, FSRC_DCT_1, sflags);
	PCG_INIT_CHECK(err == FSRC_S_OK);
	err = fsrc_ddtt_init(&pcg->dct2, L2, pcg->y, pcg->Y, FSRC_DCT_1, sflags);
	PCG_INIT_CHECK(err == FSRC_S_OK);

	if (sym == PCG_TOEP_SYM_WSHS) {
		pcg->M1 = L1;
		pcg->M2 = L2;
		pcg->dft1 = pcg->idft1 = pcg->dct1;
		pcg->dft2 = pcg->idft2 = pcg->dct2;
	}

	pcg->r = (double*)fsrc_alloc(N * sizeof(double));
	PCG_INIT_CHECK(pcg->r);
	pcg->d = (double*)fsrc_alloc(N * sizeof(double));
	PCG_INIT_CHECK(pcg->d);

	return pcg;
}

/* buffer size required for preconditioner eigenvalues */
size_t toep_pcg_precond_size(toep_pcg *pcg)
{
	return pcg->M / 2 + 1;
}

/* buffer size required by circulant matrix eigenvalues */
size_t toep_pcg_circulant_size(toep_pcg *pcg)
{
	return pcg->M + 1;
}

/* computes coefficients of the generalized Jackson kernel */
double *toep_pcg_jackson(toep_pcg *pcg, unsigned r)
{
	assert(r > 0);

	/*size_t M = pcg->N / r;*/
	size_t M = (pcg->N - 1) / r + 1;
	size_t K = pcg->M + 1;

	assert(pcg->N >= r * (M - 1) + 1);

	double *in = (double*)fsrc_alloc(K * sizeof(double));
	if(!in)
		return 0;

	double *out = pcg->Y;

	for(size_t i = 0; i < M; ++i)
		in[i] = (double)(M - i) / M;

	memset(in + M, 0, (K - M) * sizeof(double));

	fsrc_ddtt(pcg->dct2, in, out);

	double s = 1.0 / (2 * (K - 1));
	for (size_t i = 0; i < K; ++i) {
		double c = out[i];
		double t = c * s;
		for (unsigned j = 1; j < r; ++j)
			t *= c;
		out[i] = t;
	}

	fsrc_ddtt(pcg->dct2, out, in);

	size_t N = r * (M - 1) + 1;
	memset(in + N, 0, (K - N) * sizeof(double));

	return in;
}

/* compute inverse eigenvalues of the circulant Jackson preconditioner */
void toep_pcg_jackson_ev(toep_pcg *pcg, double *coef, double *t, double *ev)
{
	size_t N = pcg->N;
	size_t M = pcg->M;
	size_t K = M / 2 + 1;

	double *in = pcg->y;

	for(size_t i = 0; i < N; ++i)
		in[i] = coef[i] * t[i];

	memset(in + N, 0, (M - N) * sizeof(double));	

	/*for(size_t i = 1; i < K; ++i)*/
	for(size_t i = M - N + 1; i < K; ++i)
		in[i] += in[M - i];

	fsrc_ddtt(pcg->dct1, in, ev);

	double s = 1.0 / (2 * (K - 1));
	for(size_t i = 0; i < K; ++i)
		ev[i] = s / ev[i];
}

/* embeds t in a circulant matrix and computes its eigenvalues */
void toep_pcg_circulant_ev(toep_pcg *pcg, double *t, double *gev)
{
	size_t N = pcg->N;
	size_t K = pcg->M + 1;

	double *in = pcg->y;

	memcpy(in, t, N * sizeof(double));
	memset(in + N, 0, (K - N) * sizeof(double));

	fsrc_ddtt(pcg->dct2, in, gev);

	fsrc_dscal(K, 1.0 / (2 * (K - 1)), gev);
}

/* fast circular convolution */
static void pcg_fcc(size_t N, size_t M, fsrc_dfft dft, fsrc_dfft idft, double *y, double *Y, double *V, int s)
{	
	if(s) {
		size_t L = (N + 1) / 2;
		memset(y + L, 0, (M - L) * sizeof(double));		
		fsrc_ddtt(dft, y, Y);
		for (size_t i = 0; i < M; ++i)
			Y[i] *= V[i];
		fsrc_ddtt(idft, Y, y);
		fsrc_dcopy(N - L, y + 2 * L - N, 1, y + L, -1);
	} else {		
		memset(y + N, 0, (M - N) * sizeof(double));
		fsrc_drcdft(dft, y, (fsrc_dcomplex*)Y);
		size_t K = M / 2 + 1;
		for (size_t i = 0; i < K; ++i) {
			Y[2 * i + 0] *= V[i];
			Y[2 * i + 1] *= V[i];
		}
		fsrc_dcrdft(idft, (fsrc_dcomplex*)Y, y);
	}	
}

/* solves Tx = b */
unsigned toep_pcg_solve(toep_pcg *pcg, double *gev, double *ev, double *b, double *u, double tol, unsigned maxit)
{
	size_t N = pcg->N;

	double *z = pcg->y;
	double *r = pcg->r;
	double *d = pcg->d;

	assert(maxit > 0 && tol > 0);

	memcpy(r, b, N * sizeof(double));
	memset(u, 0, N * sizeof(double));
	memset(d, 0, N * sizeof(double));

	double t1 = 1;

	tol *= fsrc_dnrm2(N, r);

	unsigned n = 0;
	do {
		memcpy(z, r, N * sizeof(double));
		pcg_fcc(N, pcg->M1, pcg->dft1, pcg->idft1, z, pcg->Y, ev, pcg->s);
		double t1old = t1;
		t1 = fsrc_ddot(N, z, r);
		fsrc_daxpy(N, t1 / t1old, d, z);
		memcpy(d, z, N * sizeof(double));
		pcg_fcc(N, pcg->M2, pcg->dft2, pcg->idft2, z, pcg->Y, gev, pcg->s);
		double tau = t1 / fsrc_ddot(N, d, z);
		fsrc_daxpy(N, tau, d, u);
		fsrc_daxpy(N, -tau, z, r);
	} while (++n < maxit && fsrc_dnrm2(N, r) > tol);

	return n;
}

void toep_pcg_destroy(toep_pcg *pcg)
{
	if(pcg->dct1) fsrc_dfft_destroy(pcg->dct1);
	if(pcg->dct2) fsrc_dfft_destroy(pcg->dct2);

	if(!pcg->s || pcg->s != PCG_TOEP_SYM_WSHS) {
		if(pcg->dft1) fsrc_dfft_destroy(pcg->dft1);
		if(pcg->idft1) fsrc_dfft_destroy(pcg->idft1);
		if(pcg->dft2) fsrc_dfft_destroy(pcg->dft2);
		if(pcg->idft2) fsrc_dfft_destroy(pcg->idft2);
	}

	fsrc_free(pcg->y);
	fsrc_free(pcg->Y);
	fsrc_free(pcg->r);
	fsrc_free(pcg->d);

	free(pcg);
}
