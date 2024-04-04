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

fsrc_err X(rcdft_init)(X(fft) *dft, size_t N, REAL *src, X(complex) *dst, int flags)
{
	if(flags & FSRC_FFT_OPTIMIZE)
		flags = FFTW_DESTROY_INPUT | FFTW_MEASURE;
	else
		flags = FFTW_DESTROY_INPUT | FFTW_ESTIMATE;

	F(iodim) dim = { N, 1, 1 };
	*dft = (X(fft))F(plan_guru_dft_r2c)(1, &dim, 0, 0, src, dst, flags);
	if(*dft)
		return FSRC_S_OK;
	return FSRC_E_EXTERNAL;
}

fsrc_err X(crdft_init)(X(fft) *dft, size_t N, X(complex) *src, REAL *dst, int flags)
{
	if(flags & FSRC_FFT_OPTIMIZE)
		flags = FFTW_DESTROY_INPUT | FFTW_MEASURE;
	else
		flags = FFTW_DESTROY_INPUT | FFTW_ESTIMATE;

	F(iodim) dim = { N, 1, 1 };
	*dft = (X(fft))F(plan_guru_dft_c2r)(1, &dim, 0, 0, src, dst, flags);
	if(*dft)
		return FSRC_S_OK;
	return FSRC_E_EXTERNAL;
}

void X(rcdft)(X(fft) dft, REAL *src, X(complex) *dst)
{
	F(execute_dft_r2c)((F(plan))dft, src, dst);
}

void X(crdft)(X(fft) dft, X(complex) *src, REAL *dst)
{
	F(execute_dft_c2r)((F(plan))dft, src, dst);
}

fsrc_err X(dtt_init)(X(fft) *dtt, size_t N, REAL *src, REAL *dst, fsrc_dtt_kind kind, int flags)
{
	if(flags & FSRC_FFT_OPTIMIZE)
		flags = FFTW_DESTROY_INPUT | FFTW_MEASURE;
	else
		flags = FFTW_DESTROY_INPUT | FFTW_ESTIMATE;

	static const fftw_r2r_kind dtt_kind[] = {
		FFTW_REDFT00,
		FFTW_REDFT10,
		FFTW_REDFT01
	};

	fftw_iodim dim = { N, 1, 1 };
	fftw_r2r_kind type = dtt_kind[kind];
	*dtt = (X(fft))F(plan_guru_r2r)(1, &dim, 0, 0, src, dst, &type, flags);
	if(*dtt)
		return FSRC_S_OK;
	return FSRC_E_EXTERNAL;
}

void X(dtt)(X(fft) dft, REAL *src, REAL *dst)
{
	F(execute_r2r)((F(plan))dft, src, dst);
}

void X(fft_destroy)(X(fft) fft)
{
	F(destroy_plan)((F(plan))fft);	
}

#undef F__
#undef X__
#undef REAL

