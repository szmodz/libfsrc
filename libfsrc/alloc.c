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
#include <stdlib.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifndef _WIN64

void *fsrc_alloc(size_t size)
{
	void *mem = malloc(size + 16);
	if(!mem)
		return 0;

	/* the heap has to be at least sizeof(void*) aligned */
	void *p = (void*)(((intptr_t)mem + 16) & (intptr_t)-16);

	*((void**)p - 1) = mem;

	return p;
}

void fsrc_free(void *p)
{
	if(p) free(*((void**)p - 1));
}

#else

/* Win64 guarantees 16-byte heap alignment */

void *fsrc_alloc(size_t size)
{
	return malloc(size);
}

void fsrc_free(void *p)
{
	return free(p);
}

#endif


