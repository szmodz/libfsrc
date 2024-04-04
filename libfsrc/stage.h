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
#ifndef FSRC_METASTAGE_H
#define FSRC_METASTAGE_H

typedef struct fsrc_stage fsrc_stage;

typedef struct fsrc_stage_vt {
	void (*destroy)(fsrc_stage *);
	fsrc_err (*process)(fsrc_stage *);
	void (*reset)(fsrc_stage *);
} fsrc_stage_vt;

struct fsrc_stage {
	const fsrc_stage_vt *vt;

	unsigned up;
	unsigned dn;

	size_t n; /* kernel length */
};

typedef struct fsrc_iobuf {
	size_t past;
	size_t size; /* per channel */
	size_t pos;
	void *data;
} fsrc_iobuf;


#endif

