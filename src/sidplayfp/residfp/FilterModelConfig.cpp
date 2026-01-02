/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2004,2010 Dag Lem
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

#include "FilterModelConfig.h"

#include <vector>
#include <algorithm>
#include <random>

namespace reSIDfp
{

FilterModelConfig::FilterModelConfig ( double vvr, double c, double vdd, double vth, double ucox, const Spline::Point* opamp_voltage, int opamp_size )
	: C ( c )
	, Vdd ( vdd )
	, Vth ( vth )
	, Vddt ( Vdd - Vth )
	, vmin ( opamp_voltage[ 0 ].x )
	, vmax ( std::max ( Vddt, opamp_voltage[ 0 ].y ) )
	, denorm ( vmax - vmin )
	, norm ( 1.0 / denorm )
	, N16 ( norm * ( ( 1 << 16 ) - 1 ) )
	, voice_voltage_range ( vvr )
{
	// Generate random noise for dithering
	{
		std::uniform_real_distribution<double>	unif ( 0.0, 1.0 );
		std::default_random_engine		re;

		for ( auto& buf : rndBuffer )
			buf = unif ( re );
	}

	setUCox ( ucox );

	// Convert op-amp voltage transfer to 16 bit values
	std::vector<Spline::Point> scaled_voltage ( opamp_size );

	for ( auto i = 0; i < opamp_size; i++ )
	{
		scaled_voltage[ i ].x = N16 * ( opamp_voltage[ i ].x - opamp_voltage[ i ].y ) / 2.0;
		// We add 32768 to get a positive number in the range [0-65535]
		scaled_voltage[ i ].x += double ( 1u << 15 );

		scaled_voltage[ i ].y = N16 * ( opamp_voltage[ i ].x - vmin );
	}

	// Create lookup table mapping capacitor voltage to op-amp input voltage:
	Spline s ( scaled_voltage );

	for ( auto x = 0; x < ( 1 << 16 ); x++ )
	{
		const auto	out = s.evaluate ( x );
		// When interpolating outside range the first elements may be negative
		const auto	tmp = out.x > 0.0 ? out.x : 0.0;
		assert ( tmp < 65535.5 );
		opamp_rev[ x ] = uint16_t ( tmp + 0.5 );
	}
}
//-----------------------------------------------------------------------------

void FilterModelConfig::setUCox ( double new_uCox )
{
	uCox = new_uCox;
	currFactorCoeff = denorm * ( uCox / 2.0 * 1.0e-6 / C );
}
//-----------------------------------------------------------------------------

void FilterModelConfig::buildSummerTable ( OpAmp& opampModel )
{
	// The filter summer operates at n ~ 1, and has 5 fundamentally different
	// input configurations (2 - 6 input "resistors").
	//
	// Note that all "on" transistors are modeled as one. This is not
	// entirely accurate, since the input for each transistor is different,
	// and transistors are not linear components. However modeling all
	// transistors separately would be extremely costly.
	const auto	r_N16 = 1.0 / N16;

	for ( auto i = 0; i < 5; i++ )
	{
		const auto  idiv = 2 + i;        // 2 - 6 input "resistors"
		const auto  size = idiv << 16;
		const auto  n = double ( idiv );
		const auto	r_idiv = 1.0 / idiv;

		opampModel.reset ();

		for ( auto vi = 0; vi < size; vi++ )
		{
			const auto	vin = vmin + vi * r_N16 * r_idiv;	// vmin .. vmax
			summer[ i ][ vi ] = getNormalizedValue ( opampModel.solve ( n, vin ) );
		}
	}
}
//-----------------------------------------------------------------------------

void FilterModelConfig::buildMixerTable ( OpAmp& opampModel, double nRatio )
{
	// The audio mixer operates at n ~ 8/6, and has 8 fundamentally different
	// input configurations (0 - 7 input "resistors").
	//
	// All "on", transistors are modeled as one - see comments above for
	// the filter summer.
	const auto	r_N16 = 1.0 / N16;

	for ( auto i = 0; i < 8; i++ )
	{
		const auto  idiv = std::max ( 1, i );
		const auto  size = std::max ( 1, i << 16 );
		const auto  n = i * nRatio;
		const auto	r_idiv = 1.0 / idiv;

		opampModel.reset ();

		for ( auto vi = 0; vi < size; vi++ )
		{
			const auto	vin = vmin + vi * r_N16 * r_idiv;	// vmin .. vmax
			mixer[ i ][ vi ] = getNormalizedValue ( opampModel.solve ( n, vin ) );
		}
	}
}
//-----------------------------------------------------------------------------

void FilterModelConfig::buildVolumeTable ( OpAmp& opampModel, double nDivisor )
{
	// 4 bit "resistor" ladders in the audio output gain necessitate 16 gain tables.
	// From die photographs of the volume "resistor" ladders it follows that
	// gain ~ vol/12 (assuming ideal op-amps and ideal "resistors").
	const auto	r_N16 = 1.0 / N16;

	for ( auto n8 = 0; n8 < 16; n8++ )
	{
		const auto  size = 1 << 16;
		const auto  n = n8 / nDivisor;

		opampModel.reset ();

		for ( auto vi = 0; vi < size; vi++ )
		{
			const auto	vin = vmin + vi * r_N16; // vmin .. vmax
			volume[ n8 ][ vi ] = getNormalizedValue ( opampModel.solve ( n, vin ) );
		}
	}
}
//-----------------------------------------------------------------------------

void FilterModelConfig::buildResonanceTable ( OpAmp& opampModel, const double resonance_n[ 16 ] )
{
	const auto	r_N16 = 1.0 / N16;

	for ( auto n8 = 0; n8 < 16; n8++ )
	{
		const auto	size = 1 << 16;
		opampModel.reset ();

		for ( auto vi = 0; vi < size; vi++ )
		{
			const auto	vin = vmin + vi * r_N16;	// vmin .. vmax
			resonance[ n8 ][ vi ] = getNormalizedValue ( opampModel.solve ( resonance_n[ n8 ], vin ) );
		}
	}
}
//-----------------------------------------------------------------------------

} // namespace reSIDfp
