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
#ifndef FSRC_LPF_DESIGN_H
#define FSRC_LPF_DESIGN_H

/* low-pass coefs */
typedef struct fsrc_lpc { 
	size_t n;
	double *h;
} fsrc_lpc;

/* low-pass spec */
typedef struct fsrc_lps { 
	double fs;
	double fp;
	double dp;
	double ds;
	uint32_t flags;
	uint32_t pad;
} fsrc_lps;

fsrc_err fsrc_lpf_design(fsrc_lpc *lpf, const fsrc_lps *lps);

#endif

