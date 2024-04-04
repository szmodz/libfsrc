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
#ifndef FSRC_XBLAS_H
#define FSRC_XBLAS_H

double fsrc_dnrm2(ptrdiff_t n, const double *RESTRICT x);
double fsrc_ddot(ptrdiff_t n, const double *RESTRICT x, const double *RESTRICT y);

void fsrc_dscal(ptrdiff_t n, double a, double *RESTRICT x);
void fsrc_daxpy(ptrdiff_t n, double a, const double *RESTRICT x, double *RESTRICT y);

void fsrc_dxpy(ptrdiff_t n, const double *RESTRICT x, double *RESTRICT y);
void fsrc_dxmy(ptrdiff_t n, const double *RESTRICT x, double *RESTRICT y);

void fsrc_dhad(ptrdiff_t n, const double *RESTRICT x, double *RESTRICT y);

void fsrc_dcopy(ptrdiff_t n, const double *RESTRICT x, ptrdiff_t sx, double *RESTRICT y, ptrdiff_t sy);

#endif

