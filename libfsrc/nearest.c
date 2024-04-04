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
#include "nearest.h"
#include "bits.h"
#include <limits.h>

/*
	quick and dirty solution. 
*/

size_t fsrc_nearest(size_t n, size_t m, int cnt, const size_t *f, size_t *p)
{
	size_t hn = SIZE_MAX;
	size_t ln = 0;

	int l = 0;
	for(;;) {
		fsrc_ull k = FSRC_DPMUL(m, f[l]);  /* k = m * f[l] */
		if(k < n) {
			size_t kk = (size_t)k;
			if(ln < kk)
				ln = kk;
			if(l == cnt - 1) {
				m = kk;
			} else {
				p[l] = kk;
				++l;
			}
		} else if(k > n) {
			if(hn > k)
				hn = (size_t)k;
			if(--l < 0)
				break;
			m = p[l];
		} else {
			return n;
		}
	}

	if(ln == 0)
		return hn;
	else if(hn == SIZE_MAX)
		return ln;
	else if(hn - n < n - ln)
		return hn;
	else
		return ln;
}

size_t fsrc_nearest_high(size_t n, size_t m, int cnt, const size_t *f, size_t *p)
{
	size_t hn = SIZE_MAX;
	int l = 0;
	for(;;) {
		fsrc_ull k = FSRC_DPMUL(m, f[l]);  /* k = m * f[l] */
		if(k < n) {
			if(l == cnt - 1) {
				m = (size_t)k;
			} else {
				p[l] = (size_t)k;
				++l;
			}
		} else if(k > n) {
			if(hn > k)
				hn = (size_t)k;
			if(--l < 0)
				break;
			m = p[l];
		} else {
			hn = n;
			break;
		}
	}
	return hn;
}

size_t fsrc_nearest_low(size_t n, size_t m, int cnt, const size_t *f, size_t *p)
{
	size_t ln = 0;
	int l = 0;
	for(;;) {
		fsrc_ull k = FSRC_DPMUL(m, f[l]);  /* k = m * f[l] */
		if(k < n) {
			size_t kk = (size_t)k;
			if(ln < kk)
				ln = kk;
			if(l == cnt - 1) {
				m = kk;
			} else {
				p[l] = kk;
				++l;
			}
		} else if(k > n) {
			if(--l < 0)
				break;
			m = p[l];
		} else {
			ln = n;
			break;
		}
	}
	return ln;
}

