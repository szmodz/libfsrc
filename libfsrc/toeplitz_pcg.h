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
#ifndef PCG_TOEPLITZ_H
#define PCG_TOEPLITZ_H

typedef struct toep_pcg toep_pcg;

enum {
	PCG_TOEP_SYM_NONE,
	PCG_TOEP_SYM_WSHS,
	PCG_TOEP_SYM_HSHS
};

#define PCG_TOEP_SYM_MASK	((1 << 5) - 1)

#define PCG_TOEP_OPT_SOLVE	(1 << 5)
#define PCG_TOEP_OPT_EIGEN	(1 << 6)

toep_pcg *toep_pcg_init(size_t N, int opt);

/* buffer size required for preconditioner eigenvalues */
size_t toep_pcg_precond_size(toep_pcg *pcg);

/* computes coefficients of the generalized Jackson kernel */
double *toep_pcg_jackson(toep_pcg *pcg, unsigned r);

/* compute the inverse eigenvalues of the circulant Jackson preconditioner */
void toep_pcg_jackson_ev(toep_pcg *pcg, double *coef, double *t, double *ev);

/* buffer size required by circulant matrix eigenvalues */
size_t toep_pcg_circulant_size(toep_pcg *pcg);

/* embeds t in a circulant matrix and computes its eigenvalues */
void toep_pcg_circulant_ev(toep_pcg *pcg, double *t, double *gev);

/* solves Tx = b */
unsigned toep_pcg_solve(toep_pcg *pcg, double *gev, double *ev, double *b, double *x, double tol, unsigned maxit);

void toep_pcg_destroy(toep_pcg *pcg);

#endif

