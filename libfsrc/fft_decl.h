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

typedef struct X(fft_s) *X(fft);
typedef REAL X(complex)[2];

fsrc_err X(rcdft_init)(X(fft) *dft, size_t N, REAL *src, X(complex) *dst, int flags);
fsrc_err X(crdft_init)(X(fft) *dft, size_t N, X(complex) *src, REAL *dst, int flags);

void X(rcdft)(X(fft) dft, REAL *src, X(complex) *dst);
void X(crdft)(X(fft) dft, X(complex) *src, REAL *dst);

fsrc_err X(dtt_init)(X(fft) *dtt, size_t N, REAL *src, REAL *dst, fsrc_dtt_kind kind, int flags);

void X(dtt)(X(fft) dft, REAL *src, REAL *dst);

void X(fft_destroy)(X(fft) fft);

#undef X__
#undef REAL



