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
#ifndef FSRC_IFSRC_H
#define FSRC_IFSRC_H

#define __STDC_LIMIT_MACROS

#include "config.h"

#define FSRC_BUILD

#include "fsrc.h"

#ifdef _MSC_VER
#define RESTRICT __restrict
#else
#define RESTRICT restrict
#endif

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(_MSC_VER)
#include "winstdint.h"
#else
#error "don't have stdint.h"
#endif

#include <limits.h>

#define FSRC_PTR_BITS (CHAR_BIT * SIZEOF_PVOID)

#if (FSRC_PTR_BITS != 32) && (FSRC_PTR_BITS != 64)
#warning "that's weird."
#endif

#if FSRC_PTR_BITS > 32
#define LIBFSRC_64
#endif

#define FSRC_MAX_STAGES 3

#undef MIN
#undef MAX

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define FSRC_NEW(type) (type*)malloc(sizeof(type))
#define FSRC_ARRAY(type, n) (type*)malloc((n) * sizeof(type))
#define FSRC_MM_ARRAY(type, n) (type*)fsrc_alloc((n) * sizeof(type))

#define FSRC_DOWNCAST(addr, type, member) ((type*)((char*)addr - offsetof(type, member)))

#endif
