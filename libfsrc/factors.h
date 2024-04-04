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

#ifndef FSRC_FACTORS_H
#define FSRC_FACTORS_H

typedef struct prime_factor {
	uint32_t p;		/* prime factor */
	uint32_t a;		/* power */
} prime_factor;

/* no more will fit into a 32 bit unsigned int */
#define MAX_FACTORS 9 

typedef prime_factor factor_table[MAX_FACTORS];

int factoru(uint32_t n, factor_table pf);

#endif

