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

#define __STDC_LIMIT_MACROS

#include "config.h"

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(_MSC_VER)
#include "winstdint.h"
#else
#error "don't have stdint.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <fsrc.h>

#include "wave.h"

struct fsrc_wave {
	FILE *file;

	int mode;

	size_t bsize;

	uint32_t csize;

	fsrc_off coff;
	fsrc_off len;
};

#include <string.h>

/* watch the damn packing. these are all packed to 1 */
#pragma pack(1) /* should be fairly portable */

typedef struct msguid {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} msguid;

#define MSGUID_SIZE 16

#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_FLOAT 3
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE

#define WAVE_FORMAT_GUID(x)  { (x), 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } }

static const msguid wave_format_pcm = WAVE_FORMAT_GUID(WAVE_FORMAT_PCM);
static const msguid wave_format_float = WAVE_FORMAT_GUID(WAVE_FORMAT_FLOAT);

#define RIFF_HDR_SIZE 12

typedef struct riff_hdr {
	uint32_t id;
	uint32_t size;
	uint32_t fmt;	
} riff_hdr;

#define RIFF_CHUNK_SIZE 8

typedef struct riff_chunk {
	uint32_t id;
	uint32_t size;
} riff_chunk;

#define WAVE_FORMAT_SIZE		14
#define PCM_FORMAT_SIZE			16
#define WAVE_FORMAT_EX_SIZE		18
#define WAVE_FORMAT_EXTRA_SIZE	22
#define WAVE_FORMAT_EXTENSIBLE_SIZE (WAVE_FORMAT_EX_SIZE + WAVE_FORMAT_EXTRA_SIZE)

typedef struct wave_format {
	uint16_t tag;
	uint16_t chans;
	uint32_t rate; /* @4 */
	uint32_t bps;
	uint16_t align; /* @12 */
	uint16_t bits; /* PCMWAVEFORMAT */
	uint16_t size; /* WAVEFORMATEX */
	/* WAVEFORMATEXTENSIBLE  begins */
	uint16_t validbits; /* @18 */
	uint32_t chanmask; /* @20 */
	msguid guid;
} wave_format;

#ifdef FSRC_BIG_ENDIAN /* this isn't ever defined currently */
#define RIFF_ID	0x52494646
#define WAVE_ID	0x57415645
#define FMT_ID	0x666d7420
#define DATA_ID	0x64617461
#else
#define RIFF_ID	0x46464952
#define WAVE_ID	0x45564157
#define FMT_ID	0x20746d66
#define DATA_ID	0x61746164
#endif

#ifdef _MSC_VER
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

static fsrc_err next_chunk(FILE *file, riff_chunk *rc, uint32_t id)
{
	for(;;) {
		if(fread(rc, RIFF_CHUNK_SIZE, 1, file) != 1)
			return FSRC_E_EXTERNAL;

		if(rc->id == id)
			return FSRC_S_OK;

		if(fseeko(file, rc->size, SEEK_CUR))
			return FSRC_E_EXTERNAL;
	}
}

static int get_float_fmt(fsrc_wave_fmt *fmt)
{
	switch(fmt->ssize)
	{
	case 4:
		fmt->sample_fmt = fsrc_f32;
		break;
	case 8:
		fmt->sample_fmt = fsrc_f64;
		break;
	default:
		return 0;
	}
	return 1;
}

static int get_pcm_fmt(fsrc_wave_fmt *fmt)
{
	switch(fmt->ssize)
	{
	case 1:
		fmt->sample_fmt = fsrc_ui8;
		break;
	case 2:
		fmt->sample_fmt = fsrc_i16;
		break;
	case 4:
		fmt->sample_fmt = fsrc_i32;
		break;
	default:
		return 0;
	}
	return 1;
}

static int deobfuscate_wave_format(wave_format *wf, size_t size, fsrc_wave_fmt *fmt)
{
	if(wf->rate == 0 || wf->chans == 0 || wf->align == 0)
		return 0;

	fmt->rate = wf->rate;
	fmt->chans = wf->chans;
	fmt->mask = 0;

	if(wf->align % wf->chans)
		return 0;
	fmt->ssize = wf->align / wf->chans;

	if(wf->tag == WAVE_FORMAT_PCM) {
		if(!get_pcm_fmt(fmt))
			return 0;
	} else if(wf->tag == WAVE_FORMAT_FLOAT) {
		if(!get_float_fmt(fmt))
			return 0;
	} else if(wf->tag == WAVE_FORMAT_EXTENSIBLE) {
		if(size < WAVE_FORMAT_EXTENSIBLE_SIZE)
			return 0;
		if(memcmp(&wf->guid, &wave_format_pcm, MSGUID_SIZE) == 0) {
			if(!get_pcm_fmt(fmt))
				return 0;
		} else if(memcmp(&wf->guid, &wave_format_float, MSGUID_SIZE) == 0) {
			if(!get_float_fmt(fmt))
				return 0;
		} else {
			return 0;
		}

		fmt->mask = wf->chanmask;

	} else {
		return 0;
	}

	return 1;
}

fsrc_err fsrc_wave_open(const char *path, fsrc_wave **pwav, fsrc_wave_fmt *fmt)
{
	FILE *file = fopen(path, "rb");
	if(!file)
		return FSRC_E_INVARG;

	riff_hdr rh;
	if(fread(&rh, RIFF_HDR_SIZE, 1, file) != 1 || rh.fmt != WAVE_ID) {
		fclose(file);
		return FSRC_E_INVARG;
	}

	fsrc_err err;
	riff_chunk rc;
	err = next_chunk(file, &rc, FMT_ID);
	if(err != FSRC_S_OK || rc.size < WAVE_FORMAT_SIZE) {
		fclose(file);
		return FSRC_E_INVARG;
	}

	wave_format wf;
	size_t fmtsize = MIN(sizeof(wave_format), rc.size);
	if(fread(&wf, fmtsize, 1, file) != 1) {
		fclose(file);
		return FSRC_E_INVARG;
	}

	if(!deobfuscate_wave_format(&wf, fmtsize, fmt)) {
		fclose(file);
		return FSRC_E_INVARG;
	}	
	
	if(rc.size > sizeof(wave_format) && fseeko(file, rc.size - sizeof(wave_format), SEEK_CUR)) {
		fclose(file);
		return FSRC_E_EXTERNAL;
	}	

	err = next_chunk(file, &rc, DATA_ID);
	if(err != FSRC_S_OK) {
		fclose(file);
		return FSRC_E_INVARG;
	}

	fsrc_wave *wav = (fsrc_wave*)malloc(sizeof(fsrc_wave));
	if(!wav) {
		fclose(file);
		return FSRC_E_EXTERNAL;
	}

	wav->bsize = fmt->ssize * fmt->chans;
	wav->csize = rc.size;

	wav->file = file;
	wav->mode = 0;

	*pwav = wav;

	return FSRC_S_OK;
}

#if 0
	uint16_t tag;
	uint16_t chans;
	uint32_t rate; /* @4 */
	uint32_t bps;
	uint16_t align; /* @12 */
	uint16_t bits; /* PCMWAVEFORMAT */
	uint16_t size; /* WAVEFORMATEX */
	/* WAVEFORMATEXTENSIBLE  begins */
	uint16_t validbits; /* @18 */
	uint32_t chanmask; /* @20 */
	msguid guid;
#endif


static uint32_t obfuscate_wave_format(fsrc_wave_fmt *fmt, wave_format *wf)
{
	wf->chans = (uint16_t)fmt->chans;
	wf->rate = (uint32_t)fmt->rate;
	wf->bps = (uint32_t)(fmt->ssize * fmt->chans * fmt->rate);
	wf->align = (uint16_t)(fmt->ssize * fmt->chans);
	wf->bits = (uint16_t)(fmt->ssize * 8);
	wf->size = 0;
	if(fmt->chans <= 2) {
		if(fmt->sample_fmt == fsrc_f32 || fmt->sample_fmt == fsrc_f64)
			wf->tag = WAVE_FORMAT_FLOAT;
		else
			wf->tag = WAVE_FORMAT_PCM;

		return PCM_FORMAT_SIZE;
	} else {
		wf->tag = WAVE_FORMAT_EXTENSIBLE;
		wf->size = WAVE_FORMAT_EXTRA_SIZE;
		wf->validbits = wf->bits;
		wf->chanmask = fmt->mask;

		if(fmt->sample_fmt == fsrc_f32 || fmt->sample_fmt == fsrc_f64)
			wf->guid = wave_format_float;
		else
			wf->guid = wave_format_pcm;

		return WAVE_FORMAT_EXTENSIBLE_SIZE;
	}
}

static int begin_data_chunk(fsrc_wave *wav)
{
	FILE *file = wav->file;
	if(fseeko(file, RIFF_CHUNK_SIZE, SEEK_CUR))
		return 0;
	return 1;
}

fsrc_err fsrc_wave_create(const char *path, fsrc_wave **pwav, fsrc_wave_fmt *fmt)
{
	FILE *file = fopen(path, "wb");
	if(!file)
		return FSRC_E_INVARG;

	struct {
		riff_hdr rh; /* just write some garbage for now */
		riff_chunk fc;
		wave_format fmt;
	} hdr;

	hdr.fc.id = FMT_ID;
	hdr.fc.size = obfuscate_wave_format(fmt, &hdr.fmt);
	size_t size = RIFF_HDR_SIZE + RIFF_CHUNK_SIZE + hdr.fc.size;
	if(fwrite(&hdr, size , 1, file) != 1) {
		fclose(file);
		return FSRC_E_EXTERNAL;
	}

	fsrc_wave *wav = (fsrc_wave*)malloc(sizeof(fsrc_wave));
	if(!wav) {
		fclose(file);
		return FSRC_E_EXTERNAL;
	}

	wav->file = file;
	wav->mode = 1;
	wav->bsize = fmt->ssize * fmt->chans;
	wav->csize = 0;
	wav->coff = size;
	wav->len = size;

	if(!begin_data_chunk(wav)) {
		fclose(file);
		return FSRC_E_EXTERNAL;
	}

	*pwav = wav;

	return FSRC_S_OK;
}

size_t fsrc_wave_read(fsrc_wave *wav, void *buf, size_t count)
{
	riff_chunk rc;
	size_t got = 0;
	FILE *file = wav->file;
	size_t bsize = wav->bsize;
	char *dst = (char*)buf;
	for(;;) {
		if(wav->csize < bsize) {			
			if(wav->csize && fseeko(file, wav->csize, SEEK_CUR))
				return got;
			if(next_chunk(file, &rc, DATA_ID) != FSRC_S_OK)
				return got;
			wav->csize = rc.size;
		}

		size_t rdcount = MIN(wav->csize / bsize, count);
		size_t ret = fread(dst, bsize, rdcount, file);
		size_t rdsize = ret * bsize;
		wav->csize = (uint32_t)(wav->csize - rdsize);
		dst += rdsize;
		got += ret;
		count -= ret;
		if(count == 0 || ret < rdcount) {
			return got;
		}
	}
}

static int update_chunk(fsrc_wave *wav)
{
	riff_chunk rc;
	FILE *file = wav->file;

	fsrc_off off = ftello(file);
	if(off < 0)
		return 0;

	if(fseeko(file, wav->coff, SEEK_SET))
		return 0;

	rc.id = DATA_ID;
	rc.size = wav->csize;

	if(fwrite(&rc, RIFF_CHUNK_SIZE, 1, file) != 1)
		return 0;

	if(fseeko(file, off, SEEK_SET))
		return 0;

	wav->len += RIFF_CHUNK_SIZE + wav->csize;

	wav->coff = off;
	wav->csize = 0;

	return 1;
}

fsrc_err fsrc_wave_close(fsrc_wave *wav)
{
	fsrc_err err = FSRC_S_OK;
	if(wav->mode) {
		if(!update_chunk(wav)) {
			err = FSRC_E_EXTERNAL;
		} else if(fseeko(wav->file, 0, SEEK_SET)) {
			err = FSRC_E_EXTERNAL;
		} else {
			riff_hdr rh = {
				RIFF_ID,
				(uint32_t)MIN(UINT32_MAX, wav->len - RIFF_CHUNK_SIZE),
				WAVE_ID				
			};

			if(fwrite(&rh, RIFF_HDR_SIZE, 1, wav->file) != 1)
				err = FSRC_E_EXTERNAL;
		}
	}

	fclose(wav->file);
	free(wav);
	return err;
}

size_t fsrc_wave_write(fsrc_wave *wav, void *buf, size_t count)
{
	size_t got = 0;
	FILE *file = wav->file;
	size_t bsize = wav->bsize;
	char *dst = (char*)buf;
	for(;;) {
		if((UINT32_MAX - wav->csize) < bsize) {
			if(!update_chunk(wav))
				return got;
			if(!begin_data_chunk(wav))
				return got;
		}

		size_t wrcount = MIN(count, (UINT32_MAX - wav->csize) / bsize);
		size_t ret = fwrite(dst, bsize, wrcount, file);		
		wav->csize = (uint32_t)(wav->csize + ret * bsize);
		dst += ret * bsize;		
		got += ret;
		count -= ret;
		if(count == 0 || ret < wrcount)
			return got;
	}
}

