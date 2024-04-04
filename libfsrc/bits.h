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

#ifndef FSRC_BITS_H
#define FSRC_BITS_H

/* some of these are from http://graphics.stanford.edu/~seander/bithacks.html */

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef _MSC_VER

#if defined(_M_X64)

#include <emmintrin.h>

#pragma intrinsic(_mm_cvtsd_si64x)

static __forceinline ptrdiff_t fsrc_rndint(double d)
{
	return _mm_cvtsd_si64x(_mm_load_sd(&d));
}

#elif defined(_M_IX86)

static __forceinline ptrdiff_t fsrc_rndint(double d)
{
	ptrdiff_t n;
	__asm {
		fld d;
		fistp n;
	}
	return n;
}

#else

static __forceinline ptrdiff_t fsrc_rndint(double d)
{
	return (ptrdiff_t)(d < 0 ? d - .5 : d + .5);
}

#endif

#else

#define fsrc_rndint lrint

#endif

#if defined(_MSC_VER) && !defined(_WIN64)
/* 32-bit MSVC is stupid when it comes to 64 bit multiplications */
#pragma intrinsic(__emulu)
#define FSRC_DPMUL(a, b) __emulu(a, b)
#else
#define FSRC_DPMUL(a, b) ((fsrc_ull)(a) * (b))
#endif

static inline size_t nextpow2(size_t v)
{
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
#ifdef LIBFSRC_64
	v |= v >> 32;
#endif
	++v;
	return v;
}

/* http://chessprogramming.wikispaces.com/De+Bruijn+Sequence+Generator */

#define FSRC_DEBRUIJN32 0x077CB531U
#define FSRC_DEBRUIJN64 0x022fdd63cc95386d

extern const char fsrc_debruijn_bitpos32[32];
extern const char fsrc_debruijn_bitpos64[64];

#if defined(HAVE_BUILTIN_CTZ) 

#if SIZEOF_SIZE_T == SIZEOF_LONGLONG
#define fsrc_ctz __builtin_ctzll
#elif SIZEOF_SIZE_T == SIZEOF_LONG
#define fsrc_ctz __builtin_ctzl
#else
#define fsrc_ctz __builtin_ctz
#endif

#else

#if defined(_MSC_VER)

#pragma intrinsic(_BitScanForward)

static __forceinline int fsrc_ctz32(uint32_t v)
{
	unsigned long i;
	_BitScanForward(&i, v);
	return (int)i;
}

#else

static inline int fsrc_ctz32(uint32_t v)
{
	int32_t u = (int32_t)v;
	v = (uint32_t)(u & -u);
	v = (uint32_t)(v * FSRC_DEBRUIJN32) >> 27;
	return fsrc_debruijn_bitpos32[v];
}

#endif

#if defined(_MSC_VER) && defined(_WIN64)

#pragma intrinsic(_BitScanForward64)

static __forceinline int fsrc_ctz64(uint64_t v)
{
	unsigned long i;
	_BitScanForward64(&i, v);
	return (int)i;
}

#else 

static inline int fsrc_ctz64(uint64_t v)
{
	int64_t u = (int64_t)v;
	v = (uint64_t)(u & -u);
	v = (uint64_t)(v * FSRC_DEBRUIJN64) >> 58;
	return fsrc_debruijn_bitpos64[v];
}

#endif

#ifdef LIBFSRC_64
#define fsrc_ctz fsrc_ctz64
#else
#define fsrc_ctz fsrc_ctz32
#endif

#endif

#if 0
#include <limits.h>

#define POPCOUNT(name, T)									\
	int popcnt##name(T v)									\
	{														\
		static const T c1 = ~(T)0/3;						\
		static const T c2 = ~(T)0/15*3;						\
		static const T c3 = ~(T)0/255*15;					\
		static const T c4 = ~(T)0/255;						\
		static const int sh = (sizeof(T) - 1) * CHAR_BIT;	\
															\
		v = v - ((v >> 1) & c1);							\
		v = (v & c2) + ((v >> 2) & c2);						\
		v = (v + (v >> 4)) & c3;							\
		return (int)((v * c4) >> sh);						\
	}

POPCOUNT(u, unsigned)
POPCOUNT(ull, unsigned long long)
#endif

#if 0

#if defined(_MSC_VER)

typedef volatile long fsrc_xint;

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)

#define fsrc_atomic_inc _InterlockedIncrement
#define fsrc_atomic_dec _InterlockedDecrement

#elif defined(HAVE_SYNC_FETCH_AND_ADD)

typedef volatile int fsrc_xint;

#define fsrc_atomic_inc(v) (__sync_fetch_and_add(v, 1) + 1)
#define fsrc_atomic_dec(v) (__sync_fetch_and_add(v, -1) - 1)

#elif defined(__APPLE__)

#include <libkern/OSAtomic.h>

typedef volatile int32_t fsrc_xint;

#define fsrc_atomic_inc OSAtomicIncrement32Barrier
#define fsrc_atomic_dec OSAtomicDecrement32Barrier

#else
/* assume x86/x64 with gcc-stlye inline asm */
typedef volatile int32_t fsrc_xint;

static inline int32_t fsrc_atomic_add(volatile int32_t *a, int32_t b)
{
	__asm__ volatile ("\tlock\n\txaddl  %1, %0\n" : "+m"(*a), "+r"(b));
	return b;
}

#define fsrc_atomic_inc(v) (fsrc_atomic_add(v, 1) + 1)
#define fsrc_atomic_dec(v) (fsrc_atomic_add(v, -1) - 1)

#endif
#endif


#endif

