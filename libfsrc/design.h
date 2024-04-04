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

#ifndef FSRC_DESIGNER_H
#define FSRC_DESIGNER_H

#include "lpf_design.h"

#define FSRC_SYMMETRIC_FILTER	0x0001

typedef struct fsrc_bufsize {
	size_t past;
	size_t size;
} fsrc_bufsize;

typedef struct fsrc_stage_model {
	fsrc_ratio ratio;

	size_t n;
	double *h;

	int flags;

	/*size_t isize;
	size_t osize;*/
} fsrc_stage_model;

typedef struct fsrc_model {
	fsrc_ratio ratio;
	size_t nstages;
	fsrc_stage_model stages[FSRC_MAX_STAGES];
	fsrc_bufsize sizes[FSRC_MAX_STAGES + 1];
} fsrc_model;

#include "lpf_design.h"

fsrc_err ifsrc_design(fsrc_cache *des, fsrc_spec *spec, fsrc_model *design);
void ifsrc_model_free(fsrc_model *model);

#endif

