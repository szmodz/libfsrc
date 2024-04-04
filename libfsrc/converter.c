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
#include "formats.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static const size_t sample_size[] = {
	1, /* fsrc_ui8, */
	2, /* fsrc_i16, */
	4, /* fsrc_i32, */
	4, /* fsrc_f32, */
	8, /* fsrc_f64, */
};

struct fsrc_converter
{
	fsrc_ratio ratio;

	size_t nstages;
	size_t nchans;

	int eos;
	size_t rem;

	fsrc_iobuf bufs[FSRC_MAX_STAGES + 1];
	fsrc_stage *stages[FSRC_MAX_STAGES];

	fsrc_cvt_tbl_t icvt;
	fsrc_cvt_tbl_t ocvt;

	size_t ss;
};

fsrc_err dols_create(const fsrc_stage_model *ms, fsrc_stage **s, fsrc_iobuf *src, fsrc_iobuf *dst, size_t chans);
fsrc_err sols_create(const fsrc_stage_model *ms, fsrc_stage **s, fsrc_iobuf *src, fsrc_iobuf *dst, size_t chans);
fsrc_err dpps_create(const fsrc_stage_model *ms, fsrc_stage **s, fsrc_iobuf *src, fsrc_iobuf *dst, size_t chans);
fsrc_err spps_create(const fsrc_stage_model *ms, fsrc_stage **s, fsrc_iobuf *src, fsrc_iobuf *dst, size_t chans);

typedef fsrc_err (*fsrc_stage_ctor)(const fsrc_stage_model *, fsrc_stage **, fsrc_iobuf *, fsrc_iobuf *, size_t);

fsrc_err fsrc_create(fsrc_cache *lib, fsrc_converter **out, fsrc_spec *spec, size_t nchans)
{
	/*size_t chunks = MIN(spec->isize / spec->ratio.dn, spec->osize / spec->ratio.up);

	if(chunks == 0)
		++chunks;

	spec->isize = chunks * spec->ratio.dn;
	spec->osize = chunks * spec->ratio.dn;*/

	fsrc_model design;
	fsrc_err err = ifsrc_design(lib, spec, &design);
	if(err != FSRC_S_OK)
		return err;

	size_t nstages = design.nstages;
	fsrc_stage_model *metas = design.stages;

	fsrc_converter *src = (fsrc_converter*)malloc(sizeof(fsrc_converter));
	memset(src, 0, sizeof(fsrc_converter));
	fsrc_stage **stages = src->stages;
	fsrc_iobuf *bufs = src->bufs;

	src->ratio = design.ratio;

	/*src->isize = metas[0].isize;
	src->osize = metas[nstages - 1].osize;*/

	src->nstages = nstages;
	src->nchans = nchans;

	if(spec->flags & FSRC_DOUBLE) {
		src->icvt = fsrc_cvt_xd;
		src->ocvt = fsrc_cvt_dx;	
		src->ss = sizeof(double);
	} else {
		src->icvt = fsrc_cvt_xs;
		src->ocvt = fsrc_cvt_sx;	
		src->ss = sizeof(float);
	}	

	size_t bs = src->ss * nchans;

	fsrc_bufsize *sizes = design.sizes;
	for(size_t i = 0; i <= nstages; ++i) {
		bufs[i].past = sizes[i].past;
		bufs[i].size = sizes[i].size;
		bufs[i].data = fsrc_alloc(bufs[i].size * bs);
	}

	fsrc_stage_ctor ctor;
	if(spec->flags & FSRC_USE_FFT) {
		if(spec->flags & FSRC_DOUBLE)
			ctor = dols_create;
		else
			ctor = sols_create;
	} else {
		if(spec->flags & FSRC_DOUBLE)
			ctor = dpps_create;
		else
			ctor = spps_create;
	}

	for(size_t i = 0; i < nstages; ++i) {
		fsrc_err err = ctor(&metas[i], &stages[i], &bufs[i], &bufs[i + 1], nchans);
		assert(err == FSRC_S_OK);
	}

	ifsrc_model_free(&design);

	fsrc_reset(src);

	*out = src;

	return FSRC_S_OK;
}

void fsrc_destroy(fsrc_converter *src)
{
	for(size_t i = 0; i < src->nstages; ++i)
		src->stages[i]->vt->destroy(src->stages[i]);

	for(size_t i = 0; i <= src->nstages; ++i)
		fsrc_free(src->bufs[i].data);

	free(src);
}

fsrc_ratio fsrc_get_ratio(fsrc_converter *src)
{
	return src->ratio;
}

size_t fsrc_get_channels(fsrc_converter *src)
{
	return src->nchans;
}

void fsrc_reset(fsrc_converter *src)
{
	src->eos = 0;
	src->rem = 0;

	for(size_t i = 0; i < src->nstages; ++i)
		src->stages[i]->vt->reset(src->stages[i]);

	fsrc_iobuf *bufs = src->bufs;
	for(size_t i = 0; i <= src->nstages; ++i) {		
		size_t size = bufs[i].size * src->nchans * src->ss;
		memset(bufs[i].data, 0, size);	
		bufs[i].pos = bufs[i].past;
	}
}

size_t fsrc_end(fsrc_converter *src)
{
	if(!src->eos) {
		src->eos = 1;

		/* see how many samples are left in the buffers and compute */
		/* how many nonzero output samples will that produce in the worst case */

		fsrc_iobuf *buf = src->bufs;
		fsrc_stage **s = src->stages;
		size_t n = src->nstages;
		fsrc_ull samps = 0;
		for(size_t i = 0; i < n; ++i) {
			unsigned up = s[i]->up;
			unsigned dn = s[i]->dn;

			size_t lpf_len = s[i]->n;
			
			samps = samps + buf[i].pos - buf[i].past;
			samps = (samps * up + lpf_len + dn - 2) / dn;
		}
		src->rem = (size_t)samps + buf[n].pos;
		assert(buf[n].past == 0);
	}
	return src->rem;
}

fsrc_iolen fsrc_maxio(fsrc_converter *src)
{
	fsrc_iobuf *ibuf = &src->bufs[0];
	fsrc_iobuf *obuf = &src->bufs[src->nstages];

	fsrc_iolen ioloen = {
		ibuf->size - ibuf->past,
		obuf->size
	};

	return ioloen;
}

size_t fsrc_read(fsrc_converter *src, const fsrc_bufdesc *desc)
{
	if(src->eos)
		return 0;

	fsrc_iobuf *buf = &src->bufs[0];
	size_t size = MIN(buf->size - buf->pos, desc->size);
	if(size == 0)
		return 0;
	
	fsrc_cvt_t cvt_proc = src->icvt[desc->fmt][0];

	char *d = (char*)buf->data + buf->pos * src->ss;
	size_t ds = buf->size * src->ss;

	size_t ss = sample_size[desc->fmt];
	char *s = (char*)desc->data;
	for(size_t i = 0; i < src->nchans; ++i) {
		cvt_proc(s, src->nchans, d, 1, size);
		s += ss;
		d += ds;
	}
	
	buf->pos += size;
	return size;
}

size_t fsrc_read_split(fsrc_converter *src, size_t bufsize, const fsrc_chandesc *desc)
{
	if(src->eos)
		return 0;

	fsrc_iobuf *buf = &src->bufs[0];
	size_t size = MIN(buf->size - buf->pos, bufsize);
	if(size == 0)
		return 0;

	char *d = (char*)buf->data + buf->pos * src->ss;
	size_t ds = buf->size * src->ss;

	for(size_t i = 0; i < src->nchans; ++i) {
		fsrc_cvt_t cvt_proc = src->icvt[desc[i].fmt][0];
		cvt_proc(desc[i].data, desc[i].stride, d, 1, size);
		d += ds;
	}
	
	buf->pos += size;
	return size;
}

size_t fsrc_write(fsrc_converter *src, const fsrc_bufdesc *desc)
{
	fsrc_iobuf *buf = &src->bufs[src->nstages];
	assert(buf->past == 0);

	size_t size = MIN(buf->pos, desc->size);
	if(size == 0)
		return 0;
	
	if(src->eos) {
		size = MIN(src->rem, size);
		if(size == 0)
			return 0;
		src->rem -= size;
	}	

	fsrc_cvt_t cvt_proc = src->ocvt[desc->fmt][0];

	char *d = (char*)buf->data;
	size_t ds = buf->size * src->ss;

	size_t ss = sample_size[desc->fmt];
	char *s = (char*)desc->data;
	for(size_t i = 0; i < src->nchans; ++i) {
		cvt_proc(d, 1, s, src->nchans, size);
		s += ss;
		d += ds;
	}

	buf->pos -= size;
	return size;
}

size_t fsrc_write_split(fsrc_converter *src, size_t bufsize, const fsrc_chandesc *desc)
{
	fsrc_iobuf *buf = &src->bufs[src->nstages];
	assert(buf->past == 0);

	size_t size = MIN(buf->pos, bufsize);
	if(size == 0)
		return 0;
	
	if(src->eos) {
		size = MIN(src->rem, size);
		if(size == 0)
			return 0;
		src->rem -= size;
	}	

	char *d = (char*)buf->data;
	size_t ds = buf->size * src->ss;

	for(size_t i = 0; i < src->nchans; ++i) {
		fsrc_cvt_t cvt_proc = src->ocvt[desc[i].fmt][0];
		cvt_proc(d, 1, desc[i].data, desc[i].stride, size);
		d += ds;
	}

	buf->pos -= size;
	return size;
}

static void fsrc_silence(fsrc_converter *src)
{
	fsrc_iobuf *buf = &src->bufs[0];
	size_t size = buf->size - buf->pos;
	if(size) {
		char *d = (char*)buf->data + buf->pos * src->ss;
		size_t ds = buf->size * src->ss;
		for(size_t i = 0; i < src->nchans; ++i) {
			memset(d, 0, size * src->ss);
			d += ds;			
		}			
		buf->pos = buf->size;
	}
}

fsrc_err fsrc_process(fsrc_converter *src)
{
	if(src->eos) {
		if(src->rem == 0)
			return FSRC_S_END;		
		fsrc_silence(src);
	}

	fsrc_stage **stages = src->stages;
	size_t nstages = src->nstages;
	for(size_t i = 0; i < nstages; ++i) {
		fsrc_err err = stages[i]->vt->process(stages[i]);
		if(err != FSRC_S_OK) {
			//assert(err == FSRC_S_BUFFER_EMPTY);
			return err;
		}
	}

	/*if(src->eos && src->bufs[src->nstages].pos >= src->rem)
		return FSRC_S_END;*/

	return FSRC_S_OK;
	/*return src->bufs[src->nstages].pos;*/
}

