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
#include "stage.h"
#include "design.h"
#include "fft.h"
#include "xblas.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define F_(name) F__(name)
#define F(name) F_(name)

#define X_(name) X__(name)
#define X(name) X_(name)

#define F__(name) fsrc_d ## name
#define X__(name) dols_ ## name 
#define REAL double

#include "ols_src_impl.h"

#define F__(name) fsrc_s ## name
#define X__(name) sols_ ## name 
#define REAL float

#include "ols_src_impl.h"

