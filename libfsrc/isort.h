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
#ifndef FSRC_ISORT_H
#define FSRC_ISORT_H

/* for nondecreasing order use > */

#define INSERTION_SORT(name, T, PRED)				\
void name(T arr[], size_t length)			\
{													\
	for(size_t i = 1; i < length; ++i) {			\
		size_t j = i;								\
		if(PRED(arr[j - 1], arr[j])) {				\
			T val = arr[j];							\
													\
			do arr[j] = arr[j - 1];					\
			while(--j && PRED(arr[j - 1], val));	\
													\
			arr[j] = val;							\
		}											\
	}												\
}

#endif

