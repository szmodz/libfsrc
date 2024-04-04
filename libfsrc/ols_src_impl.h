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

/*
	overlap-save sample rate conversion

	basically, to resample by a factor of L / M :
		-compute the DFT of an input block
		-periodically extend the transform L times
			(equivalent to zero stuffing in the time domain)
		-apply the filter (pointwise multiplication)
		-divide the spectrum into M parts and add them together
			(equivalent of decimation in the time domain)
		-compute inverse DFT

		the downside is that the transforms sizes need to be of the form K*M and K*L
		respectively for the input and output, where K is an arbitrary positive integer

		the rest is the same as in the usual overlap-save block filtering
		(the required number of past samples is ceil(N / L) - 1, N being the filter lenght)
*/

typedef struct X(stage) {
	const fsrc_stage_vt *vt;

	unsigned up;
	unsigned dn;

	size_t n; /* kernel lenght */

	size_t K;
	size_t Ns;
	size_t Ms;

	F(complex) *H;

	REAL *x;
	F(complex) *X;
	F(complex) *Y;

	size_t *I;
	
	F(fft) dft;
	F(fft) idft;

	fsrc_iobuf *src;
	fsrc_iobuf *dst;
	size_t chans;
} X(stage);

static void X(destroy)(fsrc_stage *s)
{
	X(stage) *ols = (X(stage)*)s;

	fsrc_free(ols->x);
	fsrc_free(ols->X);
	fsrc_free(ols->Y);
	fsrc_free(ols->H);
	free(ols->I);

	F(fft_destroy)(ols->dft);
	F(fft_destroy)(ols->idft);

	free(ols);
}

static fsrc_err X(process)(fsrc_stage *s)
{
	X(stage) *ols = (X(stage)*)s;

	size_t U = ols->up;
	size_t D = ols->dn;

	size_t K = ols->K;
	size_t N = K * D;
	size_t M = K * U;
	size_t L = N * U;

	size_t sn = ols->Ns;
	size_t dn = ols->Ms;
	assert(sn <= N && dn <= M);

	REAL *sd = (REAL*)ols->src->data;
	size_t ss = ols->src->size;
	size_t sp = ols->src->pos;
	size_t sh = ols->src->past;
	if(sp < sn)
		return FSRC_S_BUFFER_EMPTY;

	REAL *dd = (REAL*)ols->dst->data;
	size_t ds = ols->dst->size;
	size_t dp = ols->dst->pos;
	size_t dh = ols->dst->past;
	if(ds - dh < dn)
		return FSRC_S_BUFFER_FULL;

	/*assert((ss - sh) / D == (ds - dh) / U);*/

	REAL *x = (REAL*)ols->x;

	F(fft) dft = ols->dft;
	F(fft) idft = ols->idft;

	F(complex) *RESTRICT X = ols->X;
	F(complex) *RESTRICT Y = ols->Y;
	F(complex) *RESTRICT H = ols->H;

	size_t *I = ols->I;

	size_t MB = M / 2 + 1; /* number of non-redundant bins */

	size_t ch = ols->chans;
	do {
		memcpy(x, sd, sn * sizeof(REAL));
		memset(x + sn, 0, (N - sn) * sizeof(REAL));		
		memmove(sd, sd + sn - sh, (sp - sn + sh) * sizeof(REAL));

		F(rcdft)(dft, x, X);

		/* make X(n) conjugate symmetric */
		for(size_t n = N / 2 + 1; n < N; ++n) {
			X[n][0] = X[N - n][0];
			X[n][1] = -X[N - n][1];
		}

		for(size_t m = 0; m < MB; ++m) {
			REAL Ym0 = 0;
			REAL Ym1 = 0;
			
			size_t l = m;
			
			for(size_t d = 0; d < D; ++d) {
				size_t n = I[l];

				Ym0 += H[l][0] * X[n][0] - H[l][1] * X[n][1];
				Ym1 += H[l][1] * X[n][0] + H[l][0] * X[n][1];
				
				l += M;
			}

			Y[m][0] = Ym0;
			Y[m][1] = Ym1;
		}

		F(crdft)(idft, Y, x);

		memcpy(dd + dp, x, dn * sizeof(REAL));

		sd += ss;
		dd += ds;
	} while(--ch);

	ols->src->pos = sp - sn + sh;
	ols->dst->pos += dn;

	return FSRC_S_OK;
}

static void X(reset)(fsrc_stage *s)
{


}

fsrc_err X(create)(const fsrc_stage_model *ms, fsrc_stage **s, fsrc_iobuf *src, fsrc_iobuf *dst, size_t chans)
{
	static const fsrc_stage_vt ols_vt = {
		X(destroy),
		X(process),
		X(reset)
	};

	assert(src->past >= (ms->n + ms->ratio.up - 1) / ms->ratio.up - 1);

	X(stage) *ols = FSRC_NEW(X(stage));

	memset(ols, 0, sizeof(X(stage)));

	ols->vt = &ols_vt;

	unsigned U = ms->ratio.up;
	unsigned D = ms->ratio.dn;

	ols->up = U;
	ols->dn = D;

	ols->n = ms->n;

	assert((src->size - src->past) % D == 0);
	assert((dst->size - dst->past) % U == 0);

	size_t UD = (size_t)U * D;

	size_t Nh = (ms->n + U - 1) / U - 1;

	assert(Nh == src->past);

	size_t Ns = src->size - src->past;

	size_t K = ((Ns + Nh) * U + UD - 1) / UD;
	K = fsrc_fft_opt_size_high(K, FSRC_FFT_SIZE_ANY);

	size_t N = K * D;
	size_t M = K * U;
	size_t L = K * UD;

	// set input buffer size to N
	// set output buffer size to M
	// set fft size to N and M

	REAL *h = FSRC_MM_ARRAY(REAL, L);
	F(complex) *H = FSRC_MM_ARRAY(F(complex), L);

	F(fft) dft;
	fsrc_err err = F(rcdft_init)(&dft, L, h, H, 0);

	assert(Nh * U < ms->n);

	/* copy with phase shift */
	size_t Nr = ms->n - Nh * U;
	size_t Nl = Nh * U;

	double scal = (double)U / L;

	for(size_t i = 0; i < Nr; ++i) {
		h[i] = (REAL)(ms->h[Nl + i] * scal);
	}

	memset(h + Nr, 0, (L - ms->n) * sizeof(REAL));

	for(size_t i = 0; i < Nl; ++i) {
		h[L - Nl + i] = (REAL)(ms->h[i] * scal);
	}

	F(rcdft)(dft, h, H);

	F(fft_destroy)(dft);

	fsrc_free(h);

	/* make H(n) conjugate symmetric */
	for(size_t l = L / 2 + 1; l < L; ++l) {
		H[l][0] = H[L - l][0];
		H[l][1] = -H[L - l][1];
	}

	size_t *I = FSRC_ARRAY(size_t, L);
	for(size_t l = 0; l < L; ++l)
		I[l] = l % N;

	ols->K = K;
	ols->Ns = src->size;
	ols->Ms = dst->size - dst->past;

	ols->H = H;

	ols->x = FSRC_MM_ARRAY(REAL, MAX(N, M));
	ols->X = FSRC_MM_ARRAY(F(complex), N);
	ols->Y = FSRC_MM_ARRAY(F(complex), M);
	ols->I = I;

	err = F(rcdft_init)(&ols->dft, N, ols->x, ols->X, 0);
	err = F(crdft_init)(&ols->idft, M, ols->X, ols->x, 0);

	ols->src = src;
	ols->dst = dst;
	ols->chans = chans;

	*s = (fsrc_stage*)ols;

	return FSRC_S_OK;
}

#undef F__
#undef X__
#undef REAL

