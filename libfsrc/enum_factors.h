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

#ifndef FSRC_DECOMPOSE_H
#define FSRC_DECOMPOSE_H

#include <stddef.h>

#include "rational.h"

typedef void (*enum_proc_t)(void *, const fsrc_rational *, size_t);

/*
	enumerates all fsrc_rational sequences r[j] of length N, 0 <= j < N
	which are factorizations of r=p/q, p > q satisfying:
		for 0 < M <= N
			p / Product(r, M) >= q
	where Product(r, M) denotes a product of the first M elements of r

	the cost func is called for each factorization
*/
int fsrc_enum_factors(int nmax, fsrc_rational r, enum_proc_t cf, void *arg);

#endif

