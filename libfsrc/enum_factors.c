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
#include "enum_factors.h"
#include "qpermute.h"
#include "qfactors.h"
#include "isort.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>

/*
	the lengths involved are small,
	so insertion sort should be the best (fastest) choice
*/
INSERTION_SORT(risort, fsrc_rational, RATIO_GT)

static int check_factors(fsrc_rational *q, int len, unsigned num, unsigned den)
{
	unsigned n = num;
	for(int i = 0; i < len - 1; ++i) {
		n = n / q[i].num * q[i].den;
		if(n < den)
			return 0;
	}
	assert(n / q[len - 1].num * q[len - 1].den == den);
	return 1;
}

static int next_solution(fsrc_rational *q, int len, unsigned num, unsigned den)
{
	int j = qpermute(q, len);
	while(j >= 0) {
		if(check_factors(q, len, num, den))
			return 1;
		j = qpermute_skipj(q, len, j);
	}
	return 0;
}

int fsrc_enum_factors(int nr, fsrc_rational r, enum_proc_t cf, void *arg)
{
	assert(r.num > r.den && nr > 1);

	rational_factors qf;
	nr = qfactors_begin(&qf, r.num, r.den, nr);
	if(nr < 2) {
		if(nr != 0)
			qfactors_finish(&qf);
		return 0;
	}
	
	int len;
	while(len = qfactors_next(&qf)) {
		fsrc_rational *rv = qf.r;

		risort(rv, len);

#ifndef NDEBUG
		assert(check_factors(rv, len, r.num, r.den));
#endif
		do cf(arg, rv, len);
		while(next_solution(rv, len, r.num, r.den));
	}

	qfactors_finish(&qf);

	return nr;
}



