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
#include "fft.h"
#include "nearest.h"

#include <fftw3.h>

#ifdef LIBFSRC_64

#define fftw_iodim fftw_iodim64
#define fftwf_iodim fftwf_iodim64

#define fftw_plan_guru_dft_r2c fftw_plan_guru64_dft_r2c
#define fftw_plan_guru_dft_c2r fftw_plan_guru64_dft_c2r
#define fftw_plan_guru_r2r fftw_plan_guru64_r2r

#define fftwf_plan_guru_dft_r2c fftwf_plan_guru64_dft_r2c
#define fftwf_plan_guru_dft_c2r fftwf_plan_guru64_dft_c2r
#define fftwf_plan_guru_r2r fftwf_plan_guru64_r2r

#endif

#define F_(name) F__(name)
#define F(name) F_(name)

#define X_(name) X__(name)
#define X(name) X_(name)

#define F__(name) fftw_ ## name
#define X__(name) fsrc_d ## name
#define REAL double

#include "fftwx_impl.h"

#define F__(name) fftwf_ ## name
#define X__(name) fsrc_s ## name
#define REAL float

#include "fftwx_impl.h"

static const size_t fft_factors[] = { 2, 3, 5, 7 };
static const int nfft_factors = sizeof(fft_factors) / sizeof(fft_factors[0]);

size_t fsrc_fft_opt_size(size_t n, int eo)
{
	size_t p[nfft_factors];
	if(eo > 0)
		return fsrc_nearest(n, 2, nfft_factors, fft_factors, p);
	else if(eo < 0)
		return fsrc_nearest(n, 1, nfft_factors - 1, fft_factors + 1, p);
	else
		return fsrc_nearest(n, 1, nfft_factors, fft_factors, p);
}

size_t fsrc_fft_opt_size_high(size_t n, int eo)
{
	size_t p[nfft_factors];
	if(eo > 0)
		return fsrc_nearest_high(n, 2, nfft_factors, fft_factors, p);
	else if(eo < 0)
		return fsrc_nearest_high(n, 1, nfft_factors - 1, fft_factors + 1, p);
	else
		return fsrc_nearest_high(n, 1, nfft_factors, fft_factors, p);
}

size_t fsrc_fft_opt_size_low(size_t n, int eo)
{
	size_t p[nfft_factors];
	if(eo > 0)
		return fsrc_nearest_low(n, 2, nfft_factors, fft_factors, p);
	else if(eo < 0)
		return fsrc_nearest_low(n, 1, nfft_factors - 1, fft_factors + 1, p);
	else
		return fsrc_nearest_low(n, 1, nfft_factors, fft_factors, p);
}

#include <math.h>

/* 
	Transform Lengths for Correlation and Convolution
	ALAN DI CENZO

	using Newton's method

	n is the filter length
*/

size_t fsrc_fft_block_size(size_t n)
{
	double y, t, s, u;	
	double r = n - 1.0;	
	double x = .9; /* should be close enough */
	for(;;) {
		t = 1 - x;
		s = r / t;
		u = log(s);
		y = t * u - x;
		if(fabs(y) < 1e-7)
			break;
		x += y / u;
	}
	return (size_t)(s + .5);
}

/*
	N: number of signal samples 
	L: block size 
*/

double fsrc_fft_block_cost(size_t N, size_t L)
{
	double l = (double)L;
	return l / N * log(l);
}
