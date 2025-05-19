/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "Dac.h"

namespace reSIDfp
{

//-----------------------------------------------------------------------------

Dac::Dac ( unsigned int bits, const double _leakage )
	: leakage ( _leakage )
	, dac ( bits )
{
}
//-----------------------------------------------------------------------------

double Dac::getOutput ( unsigned int input, const bool saturation ) const
{
	auto    dacValue = 0.0;

	for ( auto i = 0; i < int ( dac.size () ); i++ )
	{
		const auto	transistor_on = ( input & ( 1 << i ) ) != 0;
		dacValue += transistor_on ? dac[ i ] : dac[ i ] * leakage;
	}

	if ( saturation )
	{
		constexpr auto	GAIN = 1.1;
		constexpr auto	SAT = 1.1;

		dacValue = GAIN * dacValue + ( 1.0 - GAIN ) * SAT * dacValue * dacValue * dacValue;
	}

	return dacValue;
}
//-----------------------------------------------------------------------------

void Dac::kinkedDac ( const bool is6581 )
{
	const auto	dacLength = (unsigned int)( dac.size () );

	constexpr auto  R_INFINITY = 1e6;

	// Non-linearity parameter, 8580 DACs are perfectly linear
	const auto  _2R_div_R = is6581 ? 2.20 : 2.00;

	// 6581 DACs are not terminated by a 2R resistor
	const auto  term = ! is6581;

	auto	Vsum = 0.0;

	// Calculate voltage contribution by each individual bit in the R-2R ladder.
	for ( auto set_bit = 0u; set_bit < dacLength; set_bit++ )
	{
		auto    Vn = 1.0;							// Normalized bit voltage
		auto    R = 1.0;							// Normalized R
		const auto  _2R = _2R_div_R * R;			// 2R
		auto    Rn = term ? _2R : R_INFINITY;		// Rn = 2R for correct termination, INFINITY for missing termination

		auto	 bit = 0u;

		// Calculate DAC "tail" resistance by repeated parallel substitution
		for ( ; bit < set_bit; bit++ )
			Rn = ( Rn == R_INFINITY ) ? R + _2R : R + ( _2R * Rn ) / ( _2R + Rn ); // R + 2R || Rn

		// Source transformation for bit voltage.
		if ( Rn == R_INFINITY )
		{
			Rn = _2R;
		}
		else
		{
			Rn = ( _2R * Rn ) / ( _2R + Rn ); // 2R || Rn
			Vn = Vn * Rn / _2R;
		}

		// Calculate DAC output voltage by repeated source transformation from
		// the "tail".
		for ( ++bit; bit < dacLength; bit++ )
		{
			Rn += R;
			const auto	I = Vn / Rn;
			Rn = ( _2R * Rn ) / ( _2R + Rn ); // 2R || Rn
			Vn = Rn * I;
		}

		dac[ set_bit ] = Vn;
		Vsum += Vn;
	}

	for ( auto i = 0u; i < dacLength; i++ )
		dac[ i ] /= Vsum;
}
//-----------------------------------------------------------------------------

} // namespace reSIDfp
