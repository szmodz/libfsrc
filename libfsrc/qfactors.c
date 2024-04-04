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
#include "qfactors.h"
#include "isort.h"
#include <stdlib.h>

typedef struct qfactor {
	unsigned a; /* power */
	fsrc_rational q;
} qfactor;

#define QFEXP_PRED(l, r) ((l).a > (r).a)
INSERTION_SORT(qfisort, qfactor, QFEXP_PRED)

/* prime factorization of a fsrc_rational number */
static int factorq(unsigned num, unsigned den, qfactor qf[])
{
	factor_table nf, df;
	int nn = factoru(num, nf);
	int dn = factoru(den, df);

	int n = nn + dn;

	size_t np = 0;

	for(int i = 0; i < nn; ++i) {
		qf[i].a = nf[i].a;
		qf[i].q.num = nf[i].p;
		qf[i].q.den = 1;
	}

	for(int i = 0; i < dn; ++i) {
		qf[nn + i].a = df[i].a;
		qf[nn + i].q.num = 1;
		qf[nn + i].q.den = df[i].p;
	}

	return n;
}

int qfactors_begin(rational_factors *factors, unsigned num, unsigned den, int nr)
{
	qfactor qf[MAX_QFACTORS];
	int n = factorq(num, den, qf);

	/* sort by the powers */
	qfisort(qf, n);

	int u[MAX_FACTORS];
	int npows = n;
	for(int i = 0; i < n; ++i) {
		npows += qf[i].a;
		u[i] = qf[i].a;
	}

	nr = vupart_begin(&factors->parts, u, n, nr);
	if(!nr)
		return 0;

	fsrc_rational *mem = (fsrc_rational*)malloc((npows + nr) * sizeof(fsrc_rational));
	if(!mem) {
		vupart_finish(&factors->parts);
		return 0;
	}

	/* build factor power table */
	fsrc_rational **powtbl = factors->powtbl;
	fsrc_rational *qp = mem;

	for(int i = 0; i < n; ++i) {
		powtbl[i] = qp;

		unsigned m = qf[i].a;
		unsigned num = qf[i].q.num;
		unsigned den = qf[i].q.den;

		unsigned npow = 1;
		unsigned dpow = 1;

		for(unsigned j = 0; j <= m; ++j) {
			qp[j].num = npow;
			qp[j].den = dpow;

			npow *= num;
			dpow *= den;			
		}

		qp += m + 1;
	}

	factors->r = qp;

	return nr;
}

int qfactors_next(rational_factors *qf)
{
	vupart *p = &qf->parts;

	if(!vupart_next(p))
		return 0;

	int *f = p->f;
	int *c = p->c;
	int *v = p->v;

	int len = p->l + 1;

	for(int k = 0; k < len; ++k) {
		fsrc_rational p = { 1, 1 };
		for(int j = f[k]; j < f[k + 1]; ++j) {
			fsrc_rational q = qf->powtbl[c[j]][v[j]];
			p.num *= q.num;
			p.den *= q.den;
		}
		qf->r[k] = p;
	}

	return len;
}

void qfactors_finish(rational_factors *qf)
{
	vupart_finish(&qf->parts);
	free(qf->powtbl[0]);
}

