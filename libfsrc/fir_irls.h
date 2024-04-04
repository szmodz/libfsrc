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

#ifndef FIR_IRLS_H
#define FIR_IRLS_H

typedef struct fir_irls_spec {
	size_t n;
	double *h;

	size_t m;
	const double *f;
	const double *a;
	const double *w;

	double irls_tol;
	double pcg_tol;	
} fir_irls_spec;

typedef struct fir_irls_info {
	double del;
	int niter;
	int npcg;	
} fir_irls_info;

int fir_irls(const fir_irls_spec *spec, fir_irls_info *info);

#endif
