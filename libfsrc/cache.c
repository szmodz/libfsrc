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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

fsrc_err fsrc_file_ioi_create(const char *dir, int mode, const fsrc_ioi ***pvt);

struct fsrc_cache {
	const fsrc_ioi **pioi;
	fsrc_iom iom;
	intptr_t idx;
	intptr_t dat;
};

typedef struct fsrc_cache_hdr {
	uint32_t tag;
	uint32_t rev;
} fsrc_cache_hdr;

#define FSRC_CACHE_TAG 0x66737263
#define FSRC_CACHE_REV 0

typedef struct lpf_entry {
	fsrc_lps spec;
	int64_t off;
	int64_t n;
} lpf_entry;

typedef struct fsrc_cache_idx {
	fsrc_cache_hdr hdr;
	lpf_entry e[1];
} fsrc_cache_idx;

#define FSRC_CACHE_HDR_SIZE offsetof(fsrc_cache_idx, e)

typedef struct fsrc_idx_data {
	size_t n;
	fsrc_cache_idx *cd;
} fsrc_idx_data;

#define FSRC_LPS_CMP(a, b) memcmp(a, b, sizeof(fsrc_lps));

#define LPF_IDX_FILE "lpf.idx"
#define LPF_DAT_FILE "lpf.dat"

typedef struct merge_result {
	size_t n;
	fsrc_cache_idx *cd;
	size_t m;
	size_t *I;
} merge_result;

static int fsrc_lps_cmp(const void *l, const void *r)
{
	return FSRC_LPS_CMP(l, r);
}

fsrc_err fsrc_cache_create_dir(fsrc_cache **cache, const char *path, int mode, int rw)
{
	fsrc_err err;
	const fsrc_ioi **pioi;
	err = fsrc_file_ioi_create(path, mode, &pioi);
	if(err != FSRC_S_OK)
		return err;

	err = fsrc_cache_create_ioi(cache, pioi, rw);
	if(err != FSRC_S_OK) {
		(*pioi)->dispose(pioi);
	}

	return err;	
}

static fsrc_err ifsrc_cache_init(fsrc_cache *cache, const fsrc_ioi **pioi, fsrc_iom iom)
{
	intptr_t idx, dat;
	const fsrc_ioi *ioi = *pioi;
	fsrc_err err = FSRC_E_EXTERNAL;
	if(!ioi->open(pioi, &idx, LPF_IDX_FILE, iom)) {
		if(!ioi->open(pioi, &dat, LPF_DAT_FILE, iom)) {
			cache->pioi = pioi;
			cache->iom = iom;
			cache->idx = idx;
			cache->dat = dat;
			return FSRC_S_OK;
		}
		ioi->close(idx);
	}

	return err;
}

fsrc_err fsrc_cache_create_ioi(fsrc_cache **out, const fsrc_ioi **pioi, int rw)
{
	fsrc_cache *cache = (fsrc_cache*)malloc(sizeof(fsrc_cache));
	if(!cache)
		return FSRC_E_NOMEM;

	fsrc_iom iom = rw ? FSRC_IOM_RW : FSRC_IOM_READ;
	fsrc_err err = ifsrc_cache_init(cache, pioi, iom);
	if(err != FSRC_S_OK) {
		free(cache);
		return err;
	}

	*out = cache;

	return err;
}

fsrc_err fsrc_cache_clear(fsrc_cache *cache)
{
	const fsrc_ioi *ioi = *cache->pioi;
	if(ioi->lock(cache->idx))
		return FSRC_E_EXTERNAL;

	fsrc_err err = FSRC_S_OK;
	if(ioi->setsize(cache->idx, 0) || ioi->setsize(cache->dat, 0)) {
		err = FSRC_E_EXTERNAL;
	}

	ioi->unlock(cache->idx);

	return err;
}

void fsrc_cache_destroy(fsrc_cache *cache)
{
	const fsrc_ioi *ioi = *cache->pioi;
	ioi->close(cache->idx);
	ioi->close(cache->dat);
	ioi->dispose(cache->pioi);
	free(cache);
}

static fsrc_err ifsrc_index_read(fsrc_cache *cache, fsrc_idx_data *ca)
{	
	const fsrc_ioi *ioi = *cache->pioi;
	fsrc_err err = FSRC_S_OK;	
	fsrc_off off = ioi->getsize(cache->idx);
	if(off < FSRC_CACHE_HDR_SIZE || off > SIZE_MAX) {
		err = FSRC_E_INVARG;
	} else {
		size_t size = (size_t)off;
		size_t m = (size - FSRC_CACHE_HDR_SIZE) / sizeof(lpf_entry);
		size_t r = (size - FSRC_CACHE_HDR_SIZE) % sizeof(lpf_entry);
		if(r != 0 || m == 0) {
			err = FSRC_E_INVARG;
		} else {
			fsrc_cache_idx *cd = (fsrc_cache_idx*)malloc(size);
			if(!cd) {
				err = FSRC_E_NOMEM;
			} else {
				if(ioi->seek(cache->idx, 0, SEEK_SET) < 0) {
					err = FSRC_E_EXTERNAL;					
				} else if(ioi->read(cache->idx, cd, size) != size) {
					err = FSRC_E_EXTERNAL;
				} else if(cd->hdr.tag != FSRC_CACHE_TAG || cd->hdr.rev != FSRC_CACHE_REV) {
					err = FSRC_E_INVARG;
				} else {
					ca->n = m;
					ca->cd = cd;
					return FSRC_S_OK;
				}
				free(cd);
			}
		}
	}

	return err;
}



static fsrc_err ifsrc_index_merge(merge_result *mr, const lpf_entry *a, size_t na, const lpf_entry *b, size_t nb)
{
	size_t size = FSRC_CACHE_HDR_SIZE + sizeof(lpf_entry) * (na + nb);
	fsrc_cache_idx *cd = (fsrc_cache_idx*)malloc(size);
	size_t *I = (size_t*)malloc(nb * sizeof(size_t));
	if(!cd || !I) {
		free(cd);
		free(I);
		return FSRC_E_NOMEM;
	}

	assert(na != 0 && nb != 0);

	cd->hdr.tag = FSRC_CACHE_TAG;
	cd->hdr.rev = FSRC_CACHE_REV;

	lpf_entry *e = cd->e;
	size_t i = 0;
	size_t j = 0;
	size_t k = 0;
	size_t l = 0;
	for(;;) {
		int cmp = FSRC_LPS_CMP(&a[i], &b[j]);
		if(cmp <= 0) {
			e[k++] = a[i++];
			if((cmp == 0 && ++j == nb) || i == na)
				break;
		} else {
			I[l++] = k;
			e[k++] = b[j++];			
			if(j == nb)
				break;
		}
	}

	assert(j == nb || i == na);
	if(i < na) {		
		do e[k++] = a[i++];
		while(i < na);
		assert(j == nb);
	} else if(j < nb) {
		do {
			I[l++] = k;
			e[k++] = b[j++];			
		} while(j < nb);
	}
	
	mr->n = k;
	mr->cd = cd;
	mr->m = l;
	mr->I = I;

	return FSRC_S_OK;
}

size_t ifsrc_cache_get_lpfs(fsrc_cache *cache, const fsrc_lps *lps, fsrc_lpc *lpc, size_t n)
{
	if(cache == 0)
		return 0;

	const fsrc_ioi *ioi = *cache->pioi;
	if(ioi->lock(cache->idx))
		return FSRC_E_EXTERNAL;

	size_t k = 0;
	fsrc_idx_data id;
	fsrc_err err = ifsrc_index_read(cache, &id);
	if(err == FSRC_S_OK) {	
		size_t m = id.n;
		lpf_entry *e = id.cd->e;	

		double *h = 0;
		for(size_t i = 0; i < n; ++i) {				
			lpc[i].n = 0;
			lpc[i].h = 0;

			lpf_entry *fe = (lpf_entry*)
				bsearch(&lps[i], e, m, sizeof(lpf_entry), fsrc_lps_cmp);
			if(fe) {
				size_t size = fe->n * sizeof(double);
				h = (double*)fsrc_alloc(size);
				if(!h) {
					break;
				} else if(ioi->seek(cache->dat, fe->off, SEEK_SET) < 0) {
					break;
				} else if(ioi->read(cache->dat, h, size) != size) {
					break;
				} else {
					lpc[i].n = fe->n;
					lpc[i].h = h;
					h = 0;
					++k;
				}					
			}	
		}

		free(h);
		free(id.cd);
	}

	ioi->unlock(cache->idx);

	return k;
}

static fsrc_err ifsrc_cache_write(fsrc_cache *cache, fsrc_cache_idx *ci, fsrc_lpc *lpc, size_t n)
{
	const fsrc_ioi *ioi = *cache->pioi;
	fsrc_err err = FSRC_S_OK;
	if(ioi->setsize(cache->idx, 0) || ioi->setsize(cache->dat, 0)) {
		err = FSRC_E_EXTERNAL;
	} else {
		lpf_entry *e = ci->e;
		fsrc_off off = 0;

		for(size_t i = 0; i < n; ++i) {
			size_t j = (size_t)e[i].off;
			size_t size = lpc[j].n * sizeof(double);
			if(ioi->write(cache->dat, lpc[j].h, size) != size) {
				return FSRC_E_EXTERNAL;
			} 

			e[i].off = off;	
			off += size;		
		}

		size_t size = FSRC_CACHE_HDR_SIZE + sizeof(lpf_entry) * n;
		if(ioi->write(cache->idx, ci, size) != size) {
			err = FSRC_E_EXTERNAL;
		}
	}

	return err;
}

static fsrc_err ifsrc_cache_append(fsrc_cache *cache, merge_result *mr, fsrc_lpc *lpc, size_t n)
{
	const fsrc_ioi *ioi = *cache->pioi;
	fsrc_err err = FSRC_S_OK;
	
	fsrc_off off = ioi->seek(cache->dat, 0, SEEK_END);
	if(off < 0) {
		err = FSRC_E_EXTERNAL;
	} else {
		lpf_entry *e = mr->cd->e;

		for(size_t i = 0; i < mr->m; ++i) {
			size_t j = mr->I[i];
			size_t k = (size_t)e[j].off;
			size_t size = lpc[k].n * sizeof(double);
			if(ioi->write(cache->dat, lpc[k].h, size) != size) {
				return FSRC_E_EXTERNAL;
			} 

			e[j].off = off;	
			off += size;		
		}

		size_t size = FSRC_CACHE_HDR_SIZE + sizeof(lpf_entry) * mr->n;
		if(ioi->setsize(cache->idx, 0) || ioi->write(cache->idx, mr->cd, size) != size) {
			err = FSRC_E_EXTERNAL;
		}
	}
	return err;
}

fsrc_err ifsrc_cache_lpfs(fsrc_cache *cache, const fsrc_lps *lps, fsrc_lpc *lpc, size_t n)
{
	if(cache == 0 || cache->iom == FSRC_IOM_READ)
		return FSRC_E_INVARG;

	fsrc_cache_idx *ci = (fsrc_cache_idx*)malloc(FSRC_CACHE_HDR_SIZE + n * sizeof(lpf_entry));
	if(!ci)
		return FSRC_E_NOMEM;

	ci->hdr.tag = FSRC_CACHE_TAG;
	ci->hdr.rev = FSRC_CACHE_REV;

	lpf_entry *e = ci->e;
	for(size_t i = 0; i < n; ++i) {
		e[i].spec = lps[i];
		e[i].off = i;
		e[i].n = lpc[i].n;
	}

	qsort(e, n, sizeof(lpf_entry), fsrc_lps_cmp);

	const fsrc_ioi *ioi = *cache->pioi;
	fsrc_err err = FSRC_S_OK;
	if(ioi->lock(cache->idx)) {
		err = FSRC_E_EXTERNAL;
	} else {
		fsrc_idx_data id;
		err = ifsrc_index_read(cache, &id);
		if(err != FSRC_S_OK) {
			err = ifsrc_cache_write(cache, ci, lpc, n);
		} else {
			merge_result mr;
			err = ifsrc_index_merge(&mr, id.cd->e, id.n, ci->e, n);

			free(id.cd);
			free(ci);
			ci = 0;

			if(err == FSRC_S_OK) {
				err = ifsrc_cache_append(cache, &mr, lpc, n);				
				free(mr.cd);
				free(mr.I);
			}
			
		}
		ioi->unlock(cache->idx);
	}

	free(ci);

	return err;
}


static fsrc_err ifsrc_cache_append_dat(fsrc_cache *dst, fsrc_cache *src, merge_result *mr)
{
	const fsrc_ioi *dioi = *dst->pioi;
	const fsrc_ioi *sioi = *src->pioi;

	fsrc_err err = FSRC_S_OK;
	
	fsrc_off off = dioi->seek(dst->dat, 0, SEEK_END);
	if(off < 0) {
		err = FSRC_E_EXTERNAL;
	} else {
		void *h = 0;
		lpf_entry *e = mr->cd->e;
		for(size_t i = 0; i < mr->m; ++i) {
			size_t j = mr->I[i];
			size_t size = (size_t)e[j].n * sizeof(double);
			h = malloc(size);
			if(!h) {
				err = FSRC_E_NOMEM;
				break;
			} else if(sioi->seek(src->dat, e[j].off, SEEK_SET) < 0) {
				err = FSRC_E_EXTERNAL;
				break;
			} else if(sioi->read(src->dat, h, size) != size) {
				err = FSRC_E_EXTERNAL;
				break;
			} else if(dioi->write(dst->dat, h, size) != size) {
				err = FSRC_E_EXTERNAL;
				break;
			}
			free(h);
			h = 0;
			e[j].off = off;	
			off += size;		
		}
		free(h);

		if(err != FSRC_S_OK)
			return err;

		size_t size = FSRC_CACHE_HDR_SIZE + sizeof(lpf_entry) * mr->n;
		if(dioi->setsize(dst->idx, 0) || dioi->write(dst->idx, mr->cd, size) != size) {
			err = FSRC_E_EXTERNAL;
		}			
	}
	return err;
}

fsrc_err fsrc_cache_import(fsrc_cache *dst, fsrc_cache *src)
{
	const fsrc_ioi *dioi = *dst->pioi;
	const fsrc_ioi *sioi = *src->pioi;

	fsrc_err err = FSRC_E_EXTERNAL;
	if(!dioi->lock(dst->idx)) {
		if(!sioi->lock(src->idx)) {
			fsrc_idx_data did;
			err = ifsrc_index_read(dst, &did);
			if(err == FSRC_S_OK) {
				fsrc_idx_data sid;
				err = ifsrc_index_read(src, &sid);
				if(err == FSRC_S_OK) {
					merge_result mr;
					err = ifsrc_index_merge(&mr, did.cd->e, did.n, sid.cd->e, sid.n);
					if(err == FSRC_S_OK) {
						free(sid.cd);
						free(did.cd);
						did.cd = sid.cd = 0;						
						err = ifsrc_cache_append_dat(dst, src, &mr);
						free(mr.cd);
						free(mr.I);
					}
					free(sid.cd);
				}
				free(did.cd);
			}
			sioi->unlock(src->idx);
		}
		dioi->unlock(dst->idx);
	}
	return err;
}

