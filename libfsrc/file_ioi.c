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
#include "bits.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32

#include <io.h>

/* undef just in case */
#undef stat
#undef fstat
#undef lseek
#undef read
#undef write

#define stat _stat64
#define fstat _fstat64
#define lseek _lseeki64
#define read(fd, buf, size) _read(fd, buf, (unsigned)(size))
#define write(fd, buf, size) _write(fd, buf, (unsigned)(size))

#else
#include <unistd.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define OPEN_RW		(O_RDWR | O_CREAT | O_BINARY)
#define OPEN_RWD	(O_RDWR | O_CREAT | O_TRUNC | O_BINARY)
#define OPEN_R		(O_RDONLY | O_BINARY)
#define OPEN_W		(O_WRONLY | O_CREAT | O_TRUNC | O_BINARY)

typedef struct fsrc_file_ioi {
	const fsrc_ioi *vt;	

	int mode;
	size_t len;
	char path[1];
} fsrc_file_ioi;


static void fsrc_fioi_dispose(const fsrc_ioi **pioi)
{
	fsrc_file_ioi *ioi = (fsrc_file_ioi*)pioi;
	free(pioi);
}

static char *fsrc_fioi_path(fsrc_file_ioi *ioi, const char *name)
{
	size_t len = strlen(name);
	char *path = (char*)malloc(len + ioi->len + 2);
	if(!path)
		return 0;

#ifdef _WIN32
	sprintf(path, "%s\\%s", ioi->path, name);
#else
	sprintf(path, "%s/%s", ioi->path, name);
#endif
	return path;
}

static int fsrc_fioi_open(const fsrc_ioi **pioi, intptr_t *file, const char *name, fsrc_iom mode)
{
	int oflag;
	switch(mode)
	{
	case FSRC_IOM_READ:
		oflag = OPEN_R;
		break;
	case FSRC_IOM_WRITE:
		oflag = OPEN_W;
		break;
	case FSRC_IOM_RW:
		oflag = OPEN_RW;
		break;
	case FSRC_IOM_RWD:
		oflag = OPEN_RWD;
		break;
	}

	fsrc_file_ioi *ioi = (fsrc_file_ioi*)pioi;
	char *path = fsrc_fioi_path(ioi, name);
	if(!path)
		return 0;

	int fd = open(path, oflag, ioi->mode);

	free(path);
	if(fd == -1)
		return -1;

	*file = fd;

	return 0;
}

static void fsrc_fioi_close(intptr_t file)
{
	close((int)file);
}

static size_t fsrc_fioi_read(intptr_t file, void *ptr, size_t size)
{
	return read((int)file, ptr, size);
}

static size_t fsrc_fioi_write(intptr_t file, void *ptr, size_t size)
{
	return write((int)file, ptr, size);
}

static int fsrc_fioi_flush(intptr_t file)
{
	return 0; /* no buffering */
}

static fsrc_off fsrc_fioi_seek(intptr_t file, fsrc_off off, int whence)
{
	return lseek((int)file, off, whence);
}

static fsrc_off fsrc_fioi_getsize(intptr_t file)
{
	struct stat buf;
	if(fstat((int)file, &buf))
		return -1;
	return buf.st_size;
}


/* this is pathetic */

#ifdef _WIN32
#include <windows.h>

static int fsrc_fioi_setsize(intptr_t file, fsrc_off off)
{
	HANDLE hFile = (HANDLE)_get_osfhandle((int)file);
	LARGE_INTEGER li;
	li.QuadPart = off;
	if(!SetFilePointerEx(hFile, li, NULL, FILE_BEGIN))
		return -1;
	if(!SetEndOfFile(hFile))
		return -1;
	return 0;
}

static int fsrc_fioi_lock(intptr_t file)
{
	HANDLE hFile = (HANDLE)_get_osfhandle((int)file);
	OVERLAPPED ovpd = { 0 };
	if(!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xffffffff, 0xffffffff, &ovpd))
		return -1;
	return 0;
}

static void fsrc_fioi_unlock(intptr_t file)
{
	HANDLE hFile = (HANDLE)_get_osfhandle((int)file);
	UnlockFile(hFile, 0, 0, 0xffffffff, 0xffffffff);
	DWORD Err = GetLastError();
}

#else

static int fsrc_fioi_setsize(intptr_t file, fsrc_off off)
{
	if(lseek((int)file, off, SEEK_SET) == -1)
		return -1;
	return ftruncate((int)file, off);
}

static int fsrc_fioi_lock(intptr_t file)
{
	struct flock fl;

    fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */

    int ret = fcntl((int)file, F_SETLKW, &fl);  /* F_GETLK, F_SETLK, F_SETLKW */
	if(ret == -1)
		return -1;
	return 0;
}

static void fsrc_fioi_unlock(intptr_t file)
{
	struct flock fl;

    fl.l_type   = F_UNLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */
	
	fcntl((int)file, F_SETLKW, &fl);  /* F_GETLK, F_SETLK, F_SETLKW */
}

#endif


fsrc_err fsrc_file_ioi_create(const char *dir, int mode, const fsrc_ioi ***pvt)
{
	static const fsrc_ioi vt = {
		fsrc_fioi_dispose,
		fsrc_fioi_open,
		fsrc_fioi_close,
		fsrc_fioi_read,
		fsrc_fioi_write,
		fsrc_fioi_flush,
		fsrc_fioi_seek,
		fsrc_fioi_getsize,
		fsrc_fioi_setsize,
		fsrc_fioi_lock,
		fsrc_fioi_unlock,
	};

	size_t len = strlen(dir);
	if(dir[len - 1] == '\\' || dir[len - 1] == '/')
		--len;

	fsrc_file_ioi *pioi = (fsrc_file_ioi*)malloc(sizeof(fsrc_file_ioi) + len);
	if(pioi) {
		char *mem = pioi->path;	
		memcpy(mem, dir, len);
		mem[len] = 0;
		pioi->vt = &vt;
#ifndef _WIN32
		pioi->mode = mode;
#else
		pioi->mode = S_IREAD | S_IWRITE;
#endif
		pioi->len = len;
		*pvt = &pioi->vt;
		return FSRC_S_OK;
	}
	free(pioi);

	return FSRC_E_NOMEM;
}
