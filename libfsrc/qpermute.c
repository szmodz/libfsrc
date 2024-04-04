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
#include "qpermute.h"
#include <assert.h>

static int qpermutej(fsrc_rational *a, int n, int j)
{
	int k, l;
	fsrc_rational t;

	assert(n >= 2 && j < n - 1 && j >= 0);

	while(!RATIO_LT(a[j], a[j + 1]))
		if(--j < 0)
			return -1;

	l = n - 1;
	while(!RATIO_LT(a[j], a[l]))
		--l;

	t = a[j];
	a[j] = a[l];
	a[l] = t;

	k = j;
	l = n;
	while(++k < --l) {
		t = a[k];
		a[k] = a[l];
		a[l] = t;
	}

	return j;
}

int qpermute(fsrc_rational *a, int n)
{
	if(n < 2)
		return -1;

	return qpermutej(a, n, n - 2);
}

int qpermute_skipj(fsrc_rational *a, int n, int j)
{
	int k, l;
	fsrc_rational t;

	assert(n >= 2 && j < n - 1 && j >= 0);

	k = j;
	l = n;
	while(++k < --l) {
		t = a[k];
		a[k] = a[l];
		a[l] = t;
	}

	if(RATIO_LT(a[j], a[j + 1])) {
		/* max out a[j] */

		t = a[j];

		assert(!RATIO_LT(t, a[n - 1]));

		l = j;
		do {
			a[l] = a[l + 1];
			++l;
		} while(RATIO_LT(t, a[l]));

		a[l] = t;
	}

	return qpermutej(a, n, j);
}

