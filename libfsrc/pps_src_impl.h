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
	this is an implementation of the usual POLYPHASE sample rate conversion

	the way various indices are computed might be interesing
*/

/* the awesome POLYPHASE structure */
typedef struct POLYPHASE {	
	REAL *p;	/* POLYPHASE coefs */
	size_t n;	/* number of coefs */

	unsigned q;	/* source index increment */
	unsigned r;	/* next phase */
} POLYPHASE;

typedef struct X(stage) {
	const fsrc_stage_vt *vt; /* C has virtual functions! */

	unsigned up;
	unsigned dn;

	size_t n; /* kernel lenght */

	unsigned l;	/* start phase */

	POLYPHASE *pphs;

	fsrc_iobuf *src;
	fsrc_iobuf *dst;
	size_t chans;
} X(stage);

static void X(destroy)(fsrc_stage *s)
{
	X(stage) *pps = (X(stage)*)s;

	free(pps->pphs[0].p);
	free(pps->pphs);
	free(pps);
}

static fsrc_err X(process)(fsrc_stage *s)
{
	X(stage) *pps = (X(stage)*)s;

	REAL *RESTRICT sd = (REAL*)pps->src->data;
	size_t ss = pps->src->size;
	size_t sp = pps->src->pos;
	size_t sh = pps->src->past;

	if(sp <= sh)
		return FSRC_S_BUFFER_EMPTY;

	REAL *RESTRICT dd = (REAL*)pps->dst->data;
	size_t ds = pps->dst->size;
	size_t dp = pps->dst->pos;

	unsigned L = pps->up;
	unsigned M = pps->dn;

	/* how many output samples can we produce? */
	size_t mo = (size_t)((FSRC_DPMUL(sp - sh, L) + M - 1) / M);

	/* how many samples will fit into the output buffer? */
	size_t ao = ds - dp;

	size_t dn = MIN(mo, ao);
	if(dn == 0)
		return FSRC_S_BUFFER_EMPTY;

	fsrc_ull dnM = FSRC_DPMUL(dn, M) + pps->l;
	size_t sn = (size_t)(dnM / L); /* input samples used */
	size_t lr = (size_t)(dnM % L); /* final phase number */

	assert(sn <= sp);

	POLYPHASE *phase = pps->pphs;

	size_t ch = pps->chans;
	do {
		REAL *RESTRICT x = sd + sh;
		REAL *RESTRICT y = dd + dp;

		unsigned l = pps->l;	

		ptrdiff_t k = 0;	
		for(size_t j = 0; j < dn; ++j) {
			REAL *RESTRICT p = phase[l].p;
			ptrdiff_t nl = phase[l].n;

			REAL yj = 0;
			/*for(ptrdiff_t i = 0; i < nl; ++i)
				yj += p[i] * x[k - i];*/

			/* the subfilter coefs are reversed */
			REAL *RESTRICT xk = &x[k - nl + 1];
			for(ptrdiff_t i = 0; i < nl; ++i)
				yj += p[i] * xk[i];

			y[j] = yj;

			/* look ma, no division! */
			k += phase[l].q;
			l = phase[l].r;
		}

		assert(k == sn && l == lr);
		memmove(sd, sd + k, (sp - k) * sizeof(REAL));

		sd += ss;
		dd += ds;
	} while(--ch);

	pps->l = (unsigned)lr;

	pps->src->pos -= sn;
	pps->dst->pos += dn;

	return FSRC_S_OK;
}

static void X(reset)(fsrc_stage *s)
{
	X(stage) *pps = (X(stage)*)s;
	pps->l = 0;
}

fsrc_err X(create)(const fsrc_stage_model *ms, fsrc_stage **s, fsrc_iobuf *src, fsrc_iobuf *dst, size_t chans)
{
	static const fsrc_stage_vt vt = {
		X(destroy),
		X(process),
		X(reset)
	};

	assert(src->past >= (ms->n + ms->ratio.up - 1) / ms->ratio.up - 1);

	X(stage) *pps = FSRC_NEW(X(stage));

	pps->vt = &vt;

	unsigned L = ms->ratio.up;
	unsigned M = ms->ratio.dn;

	pps->up = L;
	pps->dn = M;

	pps->n = ms->n;

	pps->l = 0;

	/* TODO: investigate exploiting L-th band filters */

	REAL *p = FSRC_ARRAY(REAL, ms->n);
	POLYPHASE *pphs = FSRC_ARRAY(POLYPHASE, L);

	size_t N = ms->n;
	double *h = ms->h;
	for(unsigned l = 0; l < L; ++l) {
		/*size_t n = 0;		
		for(size_t k = l; k < N; k += L)
			p[n++] = h[k] * L;
		*/
		size_t n = (N - l + L - 1) / L;
		for(size_t k = 0; k < n; ++k)
			p[k] = (REAL)(h[(n - k - 1) * L + l] * L);

		pphs[l].p = p;
		pphs[l].n = n;

		p += n;

		pphs[l].q = (l + M) / L;
		pphs[l].r = (l + M) % L;
	}

	pps->pphs = pphs;

	/*pps->n = ms->n;*/
	pps->src = src;
	pps->dst = dst;
	pps->chans = chans;

	*s = (fsrc_stage*)pps;

	return FSRC_S_OK;
}

#undef REAL 
#undef POLYPHASE 
#undef X__


