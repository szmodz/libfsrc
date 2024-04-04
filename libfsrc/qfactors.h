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
#ifndef FSRC_QFACTORS_H
#define FSRC_QFACTORS_H

#include "rational.h"
#include "factors.h"
#include "vupart.h"

#define MAX_QFACTORS (2 * MAX_FACTORS)

typedef struct rational_factors {
	fsrc_rational *powtbl[MAX_QFACTORS];
	fsrc_rational *r;
	vupart parts;
} rational_factors;

/*
	generates all unordered factorizations of num/den
	into at most nr factors, not in any particular order
*/

int qfactors_begin(rational_factors *factors, unsigned num, unsigned den, int nr);
int qfactors_next(rational_factors *qf);
void qfactors_finish(rational_factors *qf);

#endif
