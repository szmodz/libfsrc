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
#ifndef LIBFSRC_H
#define LIBFSRC_H

#include <stddef.h>

#ifndef _MSC_VER
/* msvc doesn't have stdint.h, but has uintptr_t in stddef */
#include <stdint.h>
#endif

#if defined(_WIN32) && defined(LIBFSRC_DLL)
#if defined(FSRC_BUILD)
#define FSRC_API __declspec(dllexport)
#else
#define FSRC_API __declspec(dllimport)
#endif
#else
#define FSRC_API
#endif

#ifdef __cplusplus
#define EXTERN_C_BEGIN	extern "C" {
#define EXTERN_C_END	}
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

EXTERN_C_BEGIN

/* I hate error codes. */

typedef enum fsrc_err {
	FSRC_E_EXTERNAL = -6,	/* external routine failed */
	FSRC_E_INTERNAL,		/* an internal problem was detected */
	FSRC_E_CONVERGENCE,		/* a fancy optimization routine didn't converge */
	FSRC_E_NORSRC,			/* faild to acquire non-memory resource */
	FSRC_E_NOMEM,			/* memory allocation failed */
	FSRC_E_EXISTS,			/* this one is used only internally */
	FSRC_E_INVARG,			/* invalid argument */
	FSRC_S_OK,				/* don't kid yourself */
	FSRC_S_NOTFOUND,		/* internal */
	FSRC_S_BUFFER_EMPTY,	/* more input required for processing */
	FSRC_S_BUFFER_FULL,		/* internal buffers can't accept any more data */
	FSRC_S_END				/* returned after calling fsrc_end */
} fsrc_err;

/* perhaps it would be more useful to write accurate error descriptions into a log file */

#ifndef FSRC_BUILD

/* shuts up msvc when targeting .net */
struct fsrc_converter { int dummy; };

#endif

typedef struct fsrc_ratio {
	unsigned up; /* up/dn == output_freq/input_freq */
	unsigned dn; /* (output to input frequency ratio) */
} fsrc_ratio;

typedef unsigned long long fsrc_ull; /* seriously: unsigned long long? that's way too long. */

FSRC_API fsrc_err fsrc_freq_ratio(fsrc_ull irate, fsrc_ull orate, fsrc_ratio *or);
FSRC_API fsrc_err fsrc_real_ratio(double x, fsrc_ratio *ratio); /* x = orate / irate */

FSRC_API double fsrc_gain_db(double db);
FSRC_API double fsrc_ripple_db(double db); /* peak to peak ripple */

/* ripple ratio corresponding to b bits of precision */
#define FSRC_BIT_DEPTH(b) (1.0 / (1LL << (b)))

/* seconds to samples */
#define FSRC_SAMPLES(s, rate) (size_t)((double)(s) * (rate) + 0.5) 
/* samples to seconds */
#define FSRC_SECONDS(s, rate) ((double)(s) / (rate)) 

/* sample format */
typedef enum fsrc_fmt {
	fsrc_ui8, /* unsigned 8 bit integer */
	fsrc_i16, /* signed 16 bit integer */
	fsrc_i32, /* signed 32 bit integer */
			  /* no signed 24 bit int */
	fsrc_f32, /* 32 bit floating point */
	fsrc_f64, /* 64 bit floating point */
} fsrc_fmt;

FSRC_API void *fsrc_alloc(size_t size);	/* SIMD aligned memory allocation */
FSRC_API void fsrc_free(void *);		/* and the corresponding free */

/* 
	the I/O inteface: used by the design cache 
	
	read/write routines are either unbufferred or must flush all buffers 
	during the calls to lock / unlock. THIS MEANS READ BUFFERS TOO.

	note that the names are always simple file names, not paths

	example:
		struct my_ioi_impl {
			const fsrc_ioi *pvt; 
			... 				 // stuff
		};

		static const fsrc_ioi vt = {
			// fill in the function pointers
			...
		};

		my_ioi_impl *my_ioi = malloc(sizeof(my_ioi_impl));
		my_ioi->ioi = &vt;
		my_ioi->... = ...
		... 

		fsrc_cache *cache;
		fsrc_cache_create_ioi(&cache, &my_ioi->ioi, 1);

*/

typedef enum fsrc_iom {
	FSRC_IOM_READ,	/* must exist */
	FSRC_IOM_WRITE,	/* destroy existing contents */
	FSRC_IOM_RW,	/* create if doesn't exist */
	FSRC_IOM_RWD	/* destroy existing contents  */
} fsrc_iom;

typedef long long fsrc_off; /* long long, independently of any stupid #defines */

typedef struct fsrc_ioi fsrc_ioi;

struct fsrc_ioi {
	void (*dispose)(const fsrc_ioi **pioi); /* release resources */

	int (*open)(const fsrc_ioi **pioi, intptr_t *file, const char *name, fsrc_iom mode);
	void (*close)(intptr_t file); 
	
	size_t (*read)(intptr_t file, void *ptr, size_t size); 
	size_t (*write)(intptr_t file, void *ptr, size_t size);

	int (*flush)(intptr_t file); /* flush user buffers, if any */

	fsrc_off (*seek)(intptr_t file, fsrc_off off, int whence); /* like lseek */

	fsrc_off (*getsize)(intptr_t file);  
	int (*setsize)(intptr_t file, fsrc_off off); /* ftruncate */

	int (*lock)(intptr_t file); /* blocks until obtains exclusive access */
	void (*unlock)(intptr_t file);
};

typedef struct fsrc_cache fsrc_cache;

/*
	initialize the design cache. speeds up converter creation,
	especially at high qualities

	implemented using fsrc_cache_create_ioi

	mode (access mode applied to created files) is ignored on Windows
	rw - read/write (nonzero) or read only mode
*/
FSRC_API fsrc_err fsrc_cache_create_dir(fsrc_cache **cache, const char *path, int mode, int rw);

FSRC_API fsrc_err fsrc_cache_create_ioi(fsrc_cache **cache, const fsrc_ioi **ioi, int rw);

/* clear any cached designs */
FSRC_API fsrc_err fsrc_cache_clear(fsrc_cache *cache);

/* frees the cache object */
FSRC_API void fsrc_cache_destroy(fsrc_cache *cache);

 /*
 	merge the contents of the source cache into destination cache
*/
FSRC_API fsrc_err fsrc_cache_import(fsrc_cache *dst, fsrc_cache *src);


/* use minimum phase filters */
#define FSRC_LPF_MINPHASE	0x01

/* 
	use fft src
	the converter should be able to automatically choose
	when to use fft src in the future.
*/
#define FSRC_USE_FFT		0x02

/* use double precision */
#define FSRC_DOUBLE			0x04

typedef struct fsrc_spec {
	int version;		/* set to 0 */
	
	ptrdiff_t id;		/* set to 0 */

	fsrc_ratio fr;		/* frequency ratio */

	size_t isize;		/* maximum input buffer size. is updated. */
	size_t osize;		/* maximum output buffer size. is updated. */

	double dp;			/* passband ripple */	
	double ds;			/* stopband ripple */

	double bw;			/* passband width, range: (0, 1). 1 is Nyquist */

	int flags;			/* see above */
} fsrc_spec;

/* a couple of quality presets */
typedef enum fsrc_preset {
	FSRC_MQ_16,
	FSRC_HQ_16,
	FSRC_MQ_20,
	FSRC_HQ_20,
	FSRC_MQ_24,
	FSRC_HQ_24,
	FSRC_PUSHING_IT
} fsrc_preset;

FSRC_API void fsrc_load_preset(fsrc_ratio fr, fsrc_preset pre, fsrc_spec *spec);

/* designs the converter and caches the design for later use */
FSRC_API fsrc_err fsrc_cache_design(fsrc_cache *cache, fsrc_spec *spec);

typedef struct fsrc_converter fsrc_converter;

/*
	cache is optional
*/
FSRC_API fsrc_err fsrc_create(fsrc_cache *cache, fsrc_converter **src, fsrc_spec *spec, size_t chans);

FSRC_API void fsrc_destroy(fsrc_converter *src);

FSRC_API fsrc_ratio fsrc_get_ratio(fsrc_converter *src); 

/* get channel count */
FSRC_API size_t fsrc_get_channels(fsrc_converter *src);

/* reset internal state */
FSRC_API void fsrc_reset(fsrc_converter *src); 

 /* tell the converter it won't receive any more samples */
FSRC_API size_t fsrc_end(fsrc_converter *src);

typedef struct fsrc_iolen {
	size_t isize;
	size_t osize;
} fsrc_iolen;

/* returns the maximum number of samples processed at once */
FSRC_API fsrc_iolen fsrc_maxio(fsrc_converter *src);

typedef struct fsrc_bufdesc {
	size_t size;	/* buffer capacity in samples per channel */
	void *data;		/* interleaved buffer pointer */

	short flags;	/* set to zero */	
	fsrc_fmt fmt;	/* sample format */	
} fsrc_bufdesc;

/* pass a channel count sized array of these */
typedef struct fsrc_chandesc {
	void *data;		/* pointer to first sample */
	size_t stride;	/* distance between consecutive samples */

	short flags;	/* set to zero */	
	fsrc_fmt fmt;	/* sample format */	
} fsrc_chandesc;

/* copy data from the buffer described by desc into internal buffers */
FSRC_API size_t fsrc_read(fsrc_converter *src, const fsrc_bufdesc *desc);

/*
	all the buffers described by the fsrc_chandesc array contain at least size valid samples
*/
FSRC_API size_t fsrc_read_split(fsrc_converter *src, size_t size, const fsrc_chandesc *desc);

/* copy data from internal buffers into the buffer described by desc */
FSRC_API size_t fsrc_write(fsrc_converter *src, const fsrc_bufdesc *desc);

/*
	all the buffers described by the fsrc_chandesc array contain at least size valid samples
*/
FSRC_API size_t fsrc_write_split(fsrc_converter *src, size_t size, const fsrc_chandesc *desc);

/* do actual processing */
FSRC_API fsrc_err fsrc_process(fsrc_converter *src);

EXTERN_C_END

#endif

