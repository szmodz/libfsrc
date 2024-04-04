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
#include "xblas.h"
#include <math.h>
#include <string.h>

double fsrc_dnrm2(ptrdiff_t n, const double *RESTRICT x)
{
	return sqrt(fsrc_ddot(n, x, x));
}

double fsrc_ddot(ptrdiff_t n, const double *RESTRICT x, const double *RESTRICT y)
{
	double dot = 0;
	for(ptrdiff_t i = 0; i < n; ++i)
		dot += x[i] * y[i];
	return dot;
}

void fsrc_dscal(ptrdiff_t n, double a, double *RESTRICT x)
{
	for(ptrdiff_t i = 0; i < n; ++i)
		x[i] *= a;
}

void fsrc_daxpy(ptrdiff_t n, double a, const double *RESTRICT x, double *RESTRICT y)
{
	for(ptrdiff_t i = 0; i < n; ++i)
		y[i] += a * x[i];
}

void fsrc_dxpy(ptrdiff_t n, const double *RESTRICT x, double *RESTRICT y)
{
	for(ptrdiff_t i = 0; i < n; ++i)
		y[i] += x[i];
}

void fsrc_dxmy(ptrdiff_t n, const double *RESTRICT x, double *RESTRICT y)
{
	for(ptrdiff_t i = 0; i < n; ++i)
		y[i] -= x[i];
}

void fsrc_dhad(ptrdiff_t n, const double *RESTRICT x, double *RESTRICT y)
{
	for(ptrdiff_t i = 0; i < n; ++i)
		y[i] *= x[i];
}

void fsrc_dcopy(ptrdiff_t n, const double *RESTRICT x, ptrdiff_t sx, double *RESTRICT y, ptrdiff_t sy)
{
	if(n <= 0) return;

	if(sx == sy && sx == 1) {
		memcpy(y, x, n * sizeof(double));
	} else {
		if(sx < 0) x = &x[-sx * (n - 1)];
		if(sy < 0) y = &y[-sy * (n - 1)];

		do {
			*y = *x;
			x += sx;
			y += sy;
		} while(--n);
	}
}

