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
#include "design.h"
#include "stage.h"
#include "bits.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define X_(name) X__(name)
#define X(name) X_(name)

#define X__(name) dpps_ ## name 
#define REAL double
#define POLYPHASE dpolyphase

#include "pps_src_impl.h"

#define X__(name) spps_ ## name 
#define REAL float
#define POLYPHASE spolyphase

#include "pps_src_impl.h"



