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

#ifndef FSRC_WAVE
#define FSRC_WAVE

/*
	If you want something done well...
*/

#undef MIN
#undef MAX

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct fsrc_wave_fmt {
	unsigned chans;
	unsigned rate;
	unsigned ssize;
	unsigned mask;
	fsrc_fmt sample_fmt;
} fsrc_wave_fmt;

typedef struct fsrc_wave fsrc_wave;

fsrc_err fsrc_wave_open(const char *path, fsrc_wave **wav, fsrc_wave_fmt *fmt);
fsrc_err fsrc_wave_create(const char *path, fsrc_wave **wav, fsrc_wave_fmt *fmt);

fsrc_err fsrc_wave_close(fsrc_wave *wav);

size_t fsrc_wave_read(fsrc_wave *wav, void *buf, size_t count);
size_t fsrc_wave_write(fsrc_wave *wav, void *buf, size_t count);

/*fsrc_err fsrc_wave_seek(fsrc_wave *wav, fsrc_off doff);*/

#endif

