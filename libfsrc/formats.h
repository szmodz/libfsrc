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

#ifndef FSRC_FORMATS_H
#define FSRC_FORMATS_H

typedef void (*fsrc_cvt_t)(void *, ptrdiff_t, void *, ptrdiff_t, ptrdiff_t);

extern const fsrc_cvt_t fsrc_cvt_xd[][5];
extern const fsrc_cvt_t fsrc_cvt_xs[][5];

extern const fsrc_cvt_t fsrc_cvt_dx[][5];
extern const fsrc_cvt_t fsrc_cvt_sx[][5];

typedef const fsrc_cvt_t (*fsrc_cvt_tbl_t)[5];

typedef enum fsrc_cvt {
	fsrc_cvt_1,
	fsrc_cvt_2,
	fsrc_cvt_4,
	fsrc_cvt_6,
	fsrc_cvt_8,
	fsrc_cvt_n
} fsrc_cvt;

#endif
