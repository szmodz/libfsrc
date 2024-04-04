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

#ifndef FSRC_FFT_H
#define FSRC_FFT_H

#define FSRC_FFT_OPTIMIZE	1

typedef enum fsrc_dtt_kind {
	FSRC_DCT_1,
	FSRC_DCT_2,
	FSRC_DCT_3
} fsrc_dtt_kind;

#define X_(name) X__(name)
#define X(name) X_(name)

#define X__(name) fsrc_d ## name
#define REAL double

#include "fft_decl.h"

#define X__(name) fsrc_s ## name
#define REAL float

#include "fft_decl.h"

#undef X
#undef X_

/* 
	find optimal fft size
	eo: 0 - no restriction, > 0 - even, < 0 - odd
*/

#define FSRC_FFT_SIZE_EVEN	 1
#define FSRC_FFT_SIZE_ODD	-1
#define FSRC_FFT_SIZE_ANY	 0

size_t fsrc_fft_opt_size(size_t n, int eo);
size_t fsrc_fft_opt_size_high(size_t n, int eo);
size_t fsrc_fft_opt_size_low(size_t n, int eo);

#define FSRC_RDFT_RSIZE(N) (N) 
#define FSRC_RDFT_CSIZE(N) ((N) / 2 + 1)

#define FSRC_DCT1_ISIZE(N) (N)
#define FSRC_DCT1_OSIZE(N) (N)

#define FSRC_DCT2_ISIZE(N) (N)
#define FSRC_DCT2_OSIZE(N) (N)

#define FSRC_DCT3_ISIZE(N) (N)
#define FSRC_DCT3_OSIZE(N) (N)

#endif

