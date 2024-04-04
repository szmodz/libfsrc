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

#ifndef FSRC_QPERMUTE_H
#define FSRC_QPERMUTE_H

#include "rational.h"

/*
	next lexicographically ordered permutation
*/
int qpermute(fsrc_rational *a, int n);

/*
	given a[0]...a[j-1] a[j] a[j+1]...a[n-1]
	where a[n] and j are the values returned by a call to permute
	(which implies that	a[j] > a[j+1] <= ... <= a[n-1])
	returns the next lexicographically ordered permutation b[n]
	such that there exists i < j:
		b[k] == a[k] for k < i
		b[i] > a[i]
		b[l] <= b[l+1] for l > i
*/
int qpermute_skipj(fsrc_rational *a, int n, int j);

#endif
