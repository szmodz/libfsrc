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
#include "ifsrc.h"
#include "formats.h"

#include <limits.h>
#include <assert.h>

#define X_(n, sn, dn) fsrc_cvt ## n ## _ ## sn ## _ ## dn
#define X(n, sn, dn) X_(n, sn, dn)

#define Y__(n, sf, df) fsrc_cvt ## n ## _ ## sf ## _ ## df
#define Y_(n, sf, df) (fsrc_cvt_t)Y__(n, sf, df)

/* who needs templates? ;) */

#define CVTPROC(m, body) \
	static void X(m, SN, DN)(ST *src, ptrdiff_t ss, DT *dst, ptrdiff_t ds, ptrdiff_t n) \
	{ \
		DECL; \
		assert(ss >= (m) && ds >= (m) && n >= 0); \
		for(ptrdiff_t i = 0; i < n; ++i) { \
			body; \
		} \
	}

#define CVT(j) EXPR(src[i * ss + j], dst[i * ds + j])

#define DECL (void)0
#define EXPR(src, dst) (dst) = (src)
#define SN f32
#define ST float
#define DN f64
#define DT double
#include "formats_impl.h"

#define DECL (void)0
#define EXPR(src, dst) (dst) = (float)(src)
#define SN f64
#define ST double
#define DN f32
#define DT float
#include "formats_impl.h"

#define REAL double
#define STYPE f64
#define REAL_INT32_MAX INT32_MAX
#define CVT_XS fsrc_cvt_xd
#define CVT_SX fsrc_cvt_dx

#include "formats_inst.h"

#define REAL float
#define STYPE f32
#define REAL_INT32_MAX 0x7FFFFF80
#define CVT_XS fsrc_cvt_xs
#define CVT_SX fsrc_cvt_sx

#include "formats_inst.h"

