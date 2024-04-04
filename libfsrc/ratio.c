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
#include "ratio.h"
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>

/* http://www.dtashley.com/howtos/2007/01/best_rational_approximation/ */
void lratio_approx(fsrc_ulratio *r, fsrc_ull m)
{
	fsrc_ull num, den, rem;
	if(r->num > r->den) {
		rem = r->den;
		den = r->num;
	} else {
		rem = r->num;
		den = r->den;
	}

	fsrc_ull p1 = 1, p = 0;
	fsrc_ull q1 = 0, q = 1;

	fsrc_ull p2, q2, ak;

	do {
		num = den;
		den = rem;

		ak = num / den;
		rem = num % den;

		p2 = p1;
		p1 = p;

		p = ak * p1 + p2;

		q2 = q1;
		q1 = q;	

		q = ak * q1 + q2;

	} while(rem && q <= m);

	if(q > m) {
		/*
		ak = (m - q2) / q1;

		p2 = ak * p1 + p2;
		q2 = ak * q1 + q2;

		p/q = better(r, p1/q1, p2/q2) */

		p = p1;
		q = q1;
	}

	if(r->num > r->den) {
		r->num = q;
		r->den = p;
	} else {
		r->num = p;
		r->den = q;
	}
}

/*
	This function provides exact conversion of double precision floats
	into a ratio of two long long integers. The source must satisfy:

		1/trunc(ULLONG_MAX) <= |x| <= trunc(ULLONG_MAX)

	where trunc(n) denotes truncation to double precision float (not rounding)		

	in practice, this becomes (assuming 53 bit mantissa and 64bit long long):

		1/DBL_MAX_INT <= |x| <= DBL_MAX_INT		

		DBL_MAX_INT = 0xFFFFFFFFFFFFF800

	the current implementation requires that long long be wider than the double's mantissa
*/
int lratio_from_double(fsrc_ulratio *q, double x)
{
	x = fabs(x);
	if(x == 0) {
		q->num = 0;
		q->den = 1;
		return 1;
	}

	double y = x;
	if(y < 1)
		y = 1 / y;

	fsrc_ull num = (fsrc_ull)y;
	fsrc_ull den = 1;

	if(num != y) {
		int n;
		double m = frexp(y, &n) * (1ULL << DBL_MANT_DIG);
		num = (fsrc_ull)m;
		if(n > DBL_MANT_DIG || num != m)
			return 0;

		den = 1ULL << (DBL_MANT_DIG - n);
	}

	if(x < 1) {
		q->num = den;
		q->den = num;
	} else {
		q->num = num;
		q->den = den;
	}

	return 1;
}
