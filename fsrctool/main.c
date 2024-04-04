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
#include <fsrc.h>
#include "wave.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <sys/types.h>

#ifdef _WIN32

#define CACHE_MODE 0

#else
#include <unistd.h>
#include <sys/stat.h>

#define CACHE_MODE S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH

#endif

char *gethomedir();

int get_preset(const char *ptr, fsrc_preset *pre)
{
	if(strcmp(ptr, "mq16") == 0) {
		*pre = FSRC_MQ_16;
	} else if(strcmp(ptr, "hq16") == 0) {
		*pre = FSRC_HQ_16;
	} else if(strcmp(ptr, "mq20") == 0) {
		*pre = FSRC_MQ_20;
	} else if(strcmp(ptr, "hq20") == 0) {
		*pre = FSRC_HQ_20;
	} else if(strcmp(ptr, "mq24") == 0) {
		*pre = FSRC_MQ_24;
	} else if(strcmp(ptr, "hq24") == 0) {
		*pre = FSRC_HQ_24;
	} else if(strcmp(ptr, "push") == 0) {
		*pre = FSRC_PUSHING_IT;
	} else {
		return 1;
	}
	return 0;
}

void usage()
{
	printf(
		"Usage:\n\t"
		"fsrctool -q preset_name -i input_file -o output_file -r sample_rate\n\t"
		"fsrctool -p passband_ripple -s stopband_atten -b bandwidth -i input_file -o output_file -r sample_rate\n\n"
		"where :\n\t"
		"preset_name can be one of mq16, hq16, mq20, hq20, mq24, hq24\n\t"
		"passband_ripple is the peak-to-peak ripple in dB\n\t"
		"stopband_atten is the stopband attenuation in dB (positive)\n\t"
		"bandwidth is the passband width in (0, 1), where 1 is the Nyquist frequency\n\t"
		"sample_rate is the output rate\n");
}

int main(int argc, char *argv[])
{
	fsrc_wave_fmt fmt;
	fsrc_wave *src, *dst;
	fsrc_spec spec;
	fsrc_bufdesc idesc, odesc;
	fsrc_err err;
	size_t got, wrote, rem, isize, osize, bsize;
	unsigned irate, orate, chans, end;
	int ret;
	char *idata, *odata, *wptr, *home;
	char *ipath, *opath;
	fsrc_cache *cache;
	fsrc_preset pre;
	fsrc_converter *cvt;	
	double d;

	ret = EXIT_FAILURE;

	if(argc == 1) {
		usage();
		return ret;
	}

	memset(&spec, 0, sizeof(spec));

	ipath = opath = 0;
	irate = 0;

	int params = 0;
	int gotpre = 0;

	for(int i = 1; i < argc - 1; ++i) {
		size_t len = strlen(argv[i]);
		if(len != 2 || argv[i][0] != '-') {
			printf("invalid argument: %s\n\n", argv[i]);
			usage();
			return ret;
		}

		switch(argv[i][1])
		{
		case 'i':
			ipath = argv[++i];
			break;
		case 'o':
			opath = argv[++i];
			break;
		case 'q':
			if(get_preset(argv[++i], &pre)) {
				printf("bad preset name\n\n");
				usage();
				return ret;
			}
			gotpre = 1;
			break;
		case 'p':
			if(sscanf(argv[++i], "%lf", &d) != 1 || d <= 0 || d > 10) {
				printf("invalid passband ripple\n\n");
				usage();
				return ret;
			}
			spec.dp = fsrc_ripple_db(d);
			params |= 1;
			break;
		case 's':
			if(sscanf(argv[++i], "%lf", &d) != 1 || d <= 0 || d > 180) {
				printf("invalid stopband attenuation\n\n");
				usage();
				return ret;
			}
			spec.ds = fsrc_gain_db(-d);
			params |= 2;
			break;
		case 'b':
			if(sscanf(argv[++i], "%lf", &d) != 1 || d <= 0 || d >= 1) {
				printf("invalid bandwidth\n\n");
				usage();
				return ret;
			}
			params |= 4;
			spec.bw = d;
			break;
		case 'r':
			if(sscanf(argv[++i], "%u", &orate) != 1 || orate == 0) {
				printf("invalid sample rate\n\n");
				usage();
				return ret;
			}
			break;
		default:
			printf("invalid argument: %s\n\n", argv[i]);
			usage();
			return ret;
		}
	}

	int nerr = 0;

	if(params != 7 && !gotpre) {
		++nerr;
		printf("quality not specified\n");

		if(params) {
			if(!(params & 1))
				printf("passband ripple not specified\n");
			if(!(params & 2))
				printf("stopband ripple not specified\n");
			if(!(params & 4))
				printf("bandwidth not specified\n");
		}
	}

	if(params && gotpre) {
		++nerr;
		printf("specify either a quality preset or specs\n");
	}

	if(!ipath) {
		++nerr;
		printf("input not specified\n");
	}

	if(!opath) {
		++nerr;
		printf("output not specified\n");		
	}

	if(nerr) {
		usage();
		return ret;

	}

#if 0
	if(argc != 5) {
		printf("usage: fsrctool qual src dst rate\n");
		return ret;
	}

	if(get_preset(argv[1], &pre)) {
		printf("bad preset name\n");
		return ret;
	}
	
	orate = atoi(argv[4]);
	if(orate <= 0) {
		printf("bad rate\n");
		return ret;
	}
#endif

	cache = 0;

	home = gethomedir();
	if(!home) {
		printf("warning: cannot get home directory\n");
	} else {
		if(fsrc_cache_create_dir(&cache, home, CACHE_MODE, 1) != FSRC_S_OK) {
			printf("warning: fsrc_cache_create_dir failed\n");
		}
	}

	src = dst = 0;
	idata = 0;
	odata = 0;
	cvt = 0;
	
	err = fsrc_wave_open(ipath, &src, &fmt);
	if(err != FSRC_S_OK) {
		printf("failed to open source\n");
		goto cleanup;
	}

	irate = fmt.rate;
	if(irate == orate) {
		printf("source rate equal to target rate: nothing to do\n");
		goto cleanup;
	}

	fmt.rate = orate;

	err = fsrc_wave_create(opath, &dst, &fmt);
	if(err != FSRC_S_OK) {
		printf("failed to open destination\n");
		goto cleanup;
	}

	isize = irate;						/* 1 second long buffers */
	osize = orate;

	if(fsrc_freq_ratio(irate, orate, &spec.fr) != FSRC_S_OK) {
		printf("invalid conversion ratio\n");
		goto cleanup;
	}

	if(gotpre) fsrc_load_preset(spec.fr, pre, &spec);

	spec.isize = isize;					/* set max internal i/o buffer capacity */
	spec.osize = osize;

	/*spec.dp = fsrc_ripple_db(.0001);
	spec.ds = fsrc_gain_db(-165);		
	spec.bw = .94;*/

#if 0
	spec.dp = fsrc_ripple_db(.001);		/* peak to peak passband ripple of 0.001 dB */
	spec.ds = fsrc_gain_db(-130);		/* stopband attenuation of 130 dB */
	spec.bw = .95;						/* preserve 95% of the bandwidth below the Nyquist rate */
#endif

	spec.flags = FSRC_DOUBLE | FSRC_USE_FFT;			/* ! */

	chans = fmt.chans;
	if(fsrc_create(cache, &cvt, &spec, chans) != FSRC_S_OK) {
		printf("fsrc_create failed\n");
		goto cleanup;
	}

	bsize = chans * fmt.ssize;

	idata = (char*)fsrc_alloc(isize * bsize);
	if(!idata) {
		printf("error: no memory\n");
		goto cleanup;
	}

	odata = (char*)fsrc_alloc(osize * bsize);
	if(!odata) {
		printf("error: no memory\n");
		goto cleanup;
	}

	memset(&idesc, 0, sizeof(idesc));
	idesc.size = 0; /* input buffer write position */
	idesc.data = idata;
	idesc.fmt = fmt.sample_fmt;
	
	memset(&odesc, 0, sizeof(odesc));
	odesc.size = osize;
	odesc.data = odata;
	odesc.fmt = fmt.sample_fmt;	

	end = 0;
	for(;;) {
		if(!end) {
			rem = isize - idesc.size;
			wptr = idata + idesc.size * bsize;

			got = fsrc_wave_read(src, wptr, rem);
			idesc.size += got;

			end = got < rem;  /* flag EOF or something */		
			if(end && idesc.size == 0) {
				fsrc_end(cvt); /* tell the converter it won't get any more samples */	
			}
		}
		
		if(idesc.size) {
			got = fsrc_read(cvt, &idesc);
			assert(got); /* shouldn't be 0 at this point */
			idesc.size -= got;
			if(idesc.size) {
				/* move remaining data to the beginning of the buffer */
				memmove(idata, idata + got * bsize, idesc.size * bsize);
			} else if (end) {
				fsrc_end(cvt); /* tell the converter it won't get any more samples */	
			}
		}

		err = fsrc_process(cvt);
		if(err == FSRC_S_END) {
			assert(end);
			ret = EXIT_SUCCESS;
			break;
		}
		assert(err == FSRC_S_OK); /* shouldn't be anything else */

		got = fsrc_write(cvt, &odesc);
		assert(got); /* shouldn't be zero here  */

		wrote = fsrc_wave_write(dst, odata, got);
		if(wrote < got) {
			printf("write error\n");
			break;
		}		
	}

	/* call fsrc_reset(cvt) before using the converter object again */

cleanup:	
	fsrc_free(idata);
	fsrc_free(odata);

	if(src) fsrc_wave_close(src);
	if(dst) fsrc_wave_close(dst);
	if(cvt) fsrc_destroy(cvt);

	if(cache) fsrc_cache_destroy(cache);	
	
	free(home);

	return ret;
}

#ifdef _WIN32

#include <windows.h>
#include <shlobj.h>

#include <io.h>
#include <direct.h>


#pragma comment(lib, "shell32.lib")

char *gethomedir()
{
	char *home = (char*)malloc(MAX_PATH + 1);
	if(!home)
		return 0;

	HRESULT hr = SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, home);
	if(hr != S_OK) {
		free(home);
		return 0;
	}

	if(strcat_s(home, MAX_PATH + 1, "\\fsrc")) {
		free(home);
		return 0;
	}

	if(_mkdir(home) && errno != EEXIST) {
		free(home);
		return 0;
	}

	return home;

}
#else

#include <pwd.h>

char *gethomedir()
{
	struct passwd *pwd = getpwuid(getuid());
	size_t len = strlen(pwd->pw_dir) + sizeof("/.libfsrc");
	char *home = (char*)malloc(len);
	if(!home)
		return 0;

	sprintf(home, "%s%s", pwd->pw_dir, "/.libfsrc");

	if(mkdir(home, CACHE_MODE) && errno != EEXIST) {
		free(home);
		return 0;
	}

	return home;
}

#endif

