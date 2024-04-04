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
#include "ratio.h"
#include <assert.h>
#include <math.h>
#include <string.h>

/* ln(10) / 20 */
#define M_LN10_20 0.115129254649702284200900

fsrc_err fsrc_freq_ratio(fsrc_ull src_rate, fsrc_ull dst_rate, fsrc_ratio *or)
{
	if(!src_rate || !dst_rate)
		return FSRC_E_INVARG;	

	fsrc_ull q, r;

	q = dst_rate / src_rate;
	r = dst_rate % src_rate;

	if(q > UINT16_MAX || (q == UINT16_MAX && r != 0))
		return FSRC_E_INVARG;

	q = src_rate / dst_rate;
	r = src_rate % dst_rate;

	if(q > UINT16_MAX || (q == UINT16_MAX && r != 0))
		return FSRC_E_INVARG;

	fsrc_ulratio lr = { dst_rate, src_rate };
	lratio_approx(&lr, UINT16_MAX);

	or->up = (unsigned)lr.num;
	or->dn = (unsigned)lr.den;

	return FSRC_S_OK;
}

fsrc_err fsrc_real_ratio(double x, fsrc_ratio *ratio)
{
	if(x <= 0)
		return FSRC_E_INVARG;	

	fsrc_ulratio lr;
	if(!lratio_from_double(&lr, x))
		return FSRC_E_INVARG;

	return fsrc_freq_ratio(lr.num, lr.den, ratio);
}

double fsrc_gain_db(double db)
{
	return exp(db * M_LN10_20);
}

double fsrc_ripple_db(double db)
{
	double g = fsrc_gain_db(fabs(db));
	return (g - 1) / (g + 1);
}

void fsrc_load_preset(fsrc_ratio fr, fsrc_preset pre, fsrc_spec *spec)
{
	memset(spec, 0, sizeof(fsrc_spec));
	spec->fr = fr;
	switch(pre)
	{
	case FSRC_MQ_16:
		spec->dp = fsrc_ripple_db(.005);
		spec->ds = fsrc_gain_db(-100);		
		spec->bw = .9;
		break;
	case FSRC_HQ_16:
		spec->dp = fsrc_ripple_db(.001);
		spec->ds = fsrc_gain_db(-100);		
		spec->bw = .95;
		break;
	case FSRC_MQ_20:
		spec->dp = fsrc_ripple_db(.001);
		spec->ds = fsrc_gain_db(-125);		
		spec->bw = .90;
		break;
	case FSRC_HQ_20:
		spec->dp = fsrc_ripple_db(.001);
		spec->ds = fsrc_gain_db(-125);		
		spec->bw = .95;
		break;
	case FSRC_MQ_24:
		spec->dp = fsrc_ripple_db(.001);
		spec->ds = fsrc_gain_db(-150);		
		spec->bw = .90;
		break;
	case FSRC_HQ_24:
		spec->dp = fsrc_ripple_db(.0005);
		spec->ds = fsrc_gain_db(-150);		
		spec->bw = .95;
		break;
	case FSRC_PUSHING_IT:
		spec->dp = fsrc_ripple_db(.00001);	/* peak to peak passband ripple of 0.00001 dB */
		spec->ds = fsrc_gain_db(-170);		/* stopband attenuation of 170 dB */
		spec->bw = .98;						/* preserve 98% of the bandwidth below the Nyquist rate */
		break;
	}

}

