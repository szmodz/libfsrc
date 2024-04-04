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
#include "vupart.h"
#include <assert.h>
#include <stdlib.h>

/*
	Source:	The Art of Computer Programming Volume 4 Fascicle 3,
		7.2.1.5: Partitions of a multiset

	y[i] <= y[i + 1] for optimal operation
*/
int vupart_begin(vupart *p, const int *y, int m, int r)
{
	int n = 0;
	assert(m > 0);
	for(int i = 0; i < m; ++i) {
		assert(y[i] > 0);
		n += y[i];
	}

	if(r <= 0 || r > n)
		r = n;

	int mr = m * (r + 1);
	int *f = (int*)malloc((r + 1 + 3 * mr) * sizeof(int));
	if(!f)
		return 0;

	int *c = f + r + 1;
	int *u = c + mr;
	int *v = u + mr;

	/* M1 */
	for(int j = 0; j < m; ++j) {
		u[j] = v[j] = y[j];
		c[j] = j;		
	}

	f[0] = 0;
	f[1] = m;

	p->f = f;
	p->c = c;
	p->u = u;
	p->v = v;
	p->l = 0;
	p->r = r;

	return r;
}

int vupart_next(vupart *p)
{
	int *f = p->f;
	int *c = p->c;
	int *u = p->u;
	int *v = p->v;

	int a = f[p->l];
	int b = f[p->l + 1];

	for(;;) {
		for(;;) {
			/* M5 */
			int j = b;
			while(v[--j] == 0)				
				v[j] = u[j];

			assert(j >= a);

			--v[j];

			if(j != a || (v[j] * (p->r - p->l) >= u[j]))
				break;

			/*if(j != a || v[j] != 0)
				break;*/

			/* M6 */
			if(--p->l < 0)
				return 0;

			b = a;
			a = f[p->l];
		}

		for(;;) {
			/* M2 */
			int j = a;
			int k = b;

			assert(a < b);

			do {
				int uk = u[j] - v[j];
				if(uk < v[j]) {
					do {
						uk = u[j] - v[j];
						if(uk) {
							u[k] = uk;							
							v[k] = uk;
							c[k] = c[j];

							++k;
						}
					} while(++j < b);

					break;
				}

				assert(uk);

				u[k] = uk;				
				v[k] = v[j];
				c[k] = c[j];

				++k;
			} while(++j < b);

			/* M3 */
			if(k == b)
				return 1;

			if(p->l == p->r - 1)
				break;

			a = b;
			b = k;

			f[++p->l + 1] = b;

			assert(p->l < p->r);
		}
	}
}

void vupart_finish(vupart *p)
{
	free(p->f);
}

#if 0
void vupart_fprint(FILE *file, vupart *p, int m)
{
	int *f = p->f;
	int *c = p->c;
	int *v = p->v;

	for(int k = 0; k <= p->l; ++k) {
		int cj = 0;
		for(int j = f[k]; j < f[k + 1]; ++j) {
			while(cj < c[j]) {
				fputs("0 ", file);
				++cj;
			}
			fprintf(file, "%d ", v[j]);
			//v[j] in c[j];
			++cj;
		}

		while(cj < m) {
			fputs("0 ", file);
			++cj;
		}

		putc('\n', file);
	}
	putc('\n', file);
}

#endif

