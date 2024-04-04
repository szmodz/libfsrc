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
#ifndef FSRC_RATIO_H
#define FSRC_RATIO_H

typedef struct fsrc_ulratio {
	fsrc_ull num;
	fsrc_ull den;
} fsrc_ulratio;

int lratio_from_double(fsrc_ulratio *q, double x);

void lratio_approx(fsrc_ulratio *r, fsrc_ull m);

#endif

