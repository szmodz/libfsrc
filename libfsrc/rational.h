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
#ifndef FSRC_RATIONAL_H
#define FSRC_RATIONAL_H

typedef struct fsrc_rational {
	unsigned num;
	unsigned den;
} fsrc_rational;

#include "bits.h"

#define RATIO_GT(l, r) (FSRC_DPMUL((l).num, (r).den) > FSRC_DPMUL((r).num, (l).den))
#define RATIO_LT(l, r) (FSRC_DPMUL((l).num, (r).den) < FSRC_DPMUL((r).num, (l).den))

#endif

