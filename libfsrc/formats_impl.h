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

CVTPROC(1, CVT(0))
CVTPROC(2, CVT(0); CVT(1))
CVTPROC(4, CVT(0); CVT(1); CVT(2); CVT(3))
CVTPROC(6, CVT(0); CVT(1); CVT(2); CVT(3); CVT(4); CVT(5))
CVTPROC(8, CVT(0); CVT(1); CVT(2); CVT(3); CVT(4); CVT(5); CVT(6); CVT(7))

#undef DECL
#undef EXPR
#undef SN
#undef ST
#undef DN
#undef DT
