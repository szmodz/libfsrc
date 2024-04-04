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
#ifndef FSRC_VUPART_H
#define FSRC_VUPART_H

typedef struct vupart {
	int *f, *c, *u, *v, l, r;
} vupart;

int vupart_begin(vupart *p, const int *y, int m, int r);
int vupart_next(vupart *p);
void vupart_finish(vupart *p);

#endif
