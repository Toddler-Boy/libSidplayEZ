/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2004,2010 Dag Lem <resid@nimrod.no>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Filter8580.h"
#include "Integrator8580.h"

namespace reSIDfp
{

/**
* W/L ratio of frequency DAC bit 0, other bits are proportional.
* When no bits are selected a resistance with half W/L ratio is selected.
*/
constexpr auto	DAC_WL0 = 0.00615;

//-----------------------------------------------------------------------------

Filter8580::Filter8580 ()
	: Filter ( *FilterModelConfig8580::getInstance () )
	, fmc8580 ( *FilterModelConfig8580::getInstance () )
	, hpIntegrator ( fmc8580 )
	, bpIntegrator ( fmc8580 )
{
	// Pre-calculate all possible filter DAC values
	for ( auto fc = 0; fc < 2048; ++fc )
	{
		auto	wl = 0.0;
		auto	dacWL = DAC_WL0;

		if ( fc )
		{
			for ( auto i = 0u; i < 11; i++ )
			{
				if ( fc & ( 1 << i ) )
					wl += dacWL;

				dacWL *= 2.0;
			}
		}
		else
		{
			wl = dacWL / 2.0;
		}

		fltDac[ fc ] = wl;
	}
	setFilterCurve ( 0.5 );

	updatedCenterFrequency ();

	input ( 0 );	// extremely low digi output, helps with preventing clicks/pops
}
//-----------------------------------------------------------------------------

void Filter8580::setFilterCurve ( double curvePosition )
{
	// Adjust curvePosition (1.2 <= curvePosition <= 1.8)
 	curvePosition = 1.2 + curvePosition * 0.6;

	hpIntegrator.setV ( curvePosition );
	bpIntegrator.setV ( curvePosition );
}
//-----------------------------------------------------------------------------

} // namespace reSIDfp
