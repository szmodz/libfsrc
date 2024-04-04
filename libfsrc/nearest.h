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
#ifndef FSRC_NEAREST_H
#define FSRC_NEAREST_H

typedef struct size_pair {
	size_t l; /* nearest lower than, or SIZE_MAX */
	size_t h; /* nearest higher than, or 0 */
} size_pair;

/*
	find m which is nearest to n and is formed only from
	any multiples of factors from a specific set
	the result must also be divisible by d

	0 < n < SIZE_MAX

	cnt - number of factors 
	f - the factors
	p - a buffer of size cnt
*/

size_t fsrc_nearest(size_t n, size_t d, int cnt, const size_t *f, size_t *p);

/* returns m >= n, or SIZE_MAX */
size_t fsrc_nearest_high(size_t n, size_t d, int cnt, const size_t *f, size_t *p);

/* returns m <= n, or 0 */
size_t fsrc_nearest_low(size_t n, size_t d, int cnt, const size_t *f, size_t *p);

#endif


