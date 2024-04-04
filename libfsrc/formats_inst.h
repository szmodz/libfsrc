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

/* x -> f64 */

#define DECL static const REAL s = (REAL)(-1.0 / INT8_MIN)
#define EXPR(src, dst) (dst) = (int8_t)((src) + (uint8_t)INT8_MIN) * s
#define SN ui8
#define ST uint8_t
#define DN STYPE
#define DT REAL
#include "formats_impl.h"

#define DECL static const REAL s = (REAL)(-1.0 / INT16_MIN)
#define EXPR(src, dst) (dst) = (src) * s
#define SN i16
#define ST int16_t
#define DN STYPE
#define DT REAL
#include "formats_impl.h"

#define DECL static const REAL s = (REAL)(-1.0 / INT32_MIN)
#define EXPR(src, dst) (dst) = (src) * s
#define SN i32
#define ST int32_t
#define DN STYPE
#define DT REAL
#include "formats_impl.h"

#define DECL (void)0
#define EXPR(src, dst) (dst) = (src)
#define SN STYPE
#define ST REAL
#define DN STYPE
#define DT REAL
#include "formats_impl.h"

/* f64 -> x */

#define CLAMP(x) ((x) > onep ? onep : ((x) < onen ? onen : (x)))

#define DECL \
	static const REAL onep = INT8_MAX; \
	static const REAL onen = INT8_MIN; \
	static const REAL s = -(REAL)INT8_MIN
#define EXPR(src, dst) { REAL x = (src) * s; (dst) = (uint8_t)((int8_t)CLAMP(x) - INT8_MIN);  }
#define SN STYPE
#define ST REAL
#define DN ui8
#define DT uint8_t
#include "formats_impl.h"

#define DECL \
	static const REAL onep = (REAL)INT16_MAX; \
	static const REAL onen = (REAL)INT16_MIN; \
	static const REAL s = -(REAL)INT16_MIN
#define EXPR(src, dst) { REAL x = (src) * s; (dst) = (int16_t)CLAMP(x);  }
#define SN STYPE
#define ST REAL
#define DN i16
#define DT int16_t
#include "formats_impl.h"

#define DECL \
	static const REAL onep = (REAL)REAL_INT32_MAX; \
	static const REAL onen = (REAL)INT32_MIN; \
	static const REAL s = -(REAL)INT32_MIN
#define EXPR(src, dst) { REAL x = (src) * s; (dst) = (int32_t)CLAMP(x);  }
#define SN STYPE
#define ST REAL
#define DN i32
#define DT int32_t
#include "formats_impl.h"

#define Y(n, sf) Y_(n, sf, STYPE)

const fsrc_cvt_t CVT_XS[][5] = {
	Y(1, ui8), Y(2, ui8), Y(4, ui8), Y(6, ui8), Y(8, ui8),
	Y(1, i16), Y(2, i16), Y(4, i16), Y(6, i16), Y(8, i16),
	Y(1, i32), Y(2, i32), Y(4, i32), Y(6, i32), Y(8, i32),
	Y(1, f32), Y(2, f32), Y(4, f32), Y(6, f32), Y(8, f32),
	Y(1, f64), Y(2, f64), Y(4, f64), Y(6, f64), Y(8, f64),
};

#undef Y
#define Y(n, sf) Y_(n, STYPE, sf)

const fsrc_cvt_t CVT_SX[][5] = {
	Y(1, ui8), Y(2, ui8), Y(4, ui8), Y(6, ui8), Y(8, ui8),
	Y(1, i16), Y(2, i16), Y(4, i16), Y(6, i16), Y(8, i16),
	Y(1, i32), Y(2, i32), Y(4, i32), Y(6, i32), Y(8, i32),
	Y(1, f32), Y(2, f32), Y(4, f32), Y(6, f32), Y(8, f32),
	Y(1, f64), Y(2, f64), Y(4, f64), Y(6, f64), Y(8, f64),
};

#undef Y

#undef REAL
#undef STYPE
#undef REAL_INT32_MAX

#undef CVT_XS
#undef CVT_SX
