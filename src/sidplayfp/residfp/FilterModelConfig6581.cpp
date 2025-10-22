/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2010 Dag Lem
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

#include "FilterModelConfig6581.h"

#include <cmath>
#include <thread>

#include "Integrator6581.h"
#include "OpAmp.h"

namespace reSIDfp
{

constexpr auto	DAC_BITS = 11u;
constexpr auto	OPAMP_SIZE_6581 = 33u;

/**
* This is the SID 6581 op-amp voltage transfer function, measured on
* CAP1B/CAP1A on a chip marked MOS 6581R4AR 0687 14.
* All measured chips have op-amps with output voltages (and thus input
* voltages) within the range of 0.81V - 10.31V.
*/
constexpr Spline::Point opamp_voltage_6581[ OPAMP_SIZE_6581 ] =
{
	{  0.81, 10.31 },  // Approximate start of actual range
	{  2.40, 10.31 },
	{  2.60, 10.30 },
	{  2.70, 10.29 },
	{  2.80, 10.26 },
	{  2.90, 10.17 },
	{  3.00, 10.04 },
	{  3.10,  9.83 },
	{  3.20,  9.58 },
	{  3.30,  9.32 },
	{  3.50,  8.69 },
	{  3.70,  8.00 },
	{  4.00,  6.89 },
	{  4.40,  5.21 },
	{  4.54,  4.54 },  // Working point (vi = vo)
	{  4.60,  4.19 },
	{  4.80,  3.00 },
	{  4.90,  2.30 },  // Change of curvature
	{  4.95,  2.03 },
	{  5.00,  1.88 },
	{  5.05,  1.77 },
	{  5.10,  1.69 },
	{  5.20,  1.58 },
	{  5.40,  1.44 },
	{  5.60,  1.33 },
	{  5.80,  1.26 },
	{  6.00,  1.21 },
	{  6.40,  1.12 },
	{  7.00,  1.02 },
	{  7.50,  0.97 },
	{  8.50,  0.89 },
	{ 10.00,  0.81 },
	{ 10.31,  0.81 },  // Approximate end of actual range
};
//-----------------------------------------------------------------------------

static thread_local std::unique_ptr<FilterModelConfig6581>	instance;

FilterModelConfig6581* FilterModelConfig6581::getInstance ()
{
	if ( ! instance.get () )
		instance.reset ( new FilterModelConfig6581 () );

	return instance.get ();
}
//-----------------------------------------------------------------------------

void FilterModelConfig6581::setFilterRange ( double adjustment )
{
	adjustment = std::clamp ( adjustment, 0.0, 1.0 );

	// Get the new uCox value, in the range [1,40]
	const auto	new_uCox = ( 1.0 + 39.0 * adjustment ) * 1e-6;

	// Ignore small changes
	if ( std::abs ( uCox - new_uCox ) < 1e-12 )
		return;

	setUCox ( new_uCox );
}
//-----------------------------------------------------------------------------

void FilterModelConfig6581::setVoiceDCDrift ( double drift )
{
	/**
	* On 6581 the DC offset varies between ~5.0V and ~5.214V depending on
	* the envelope value.
	*/
	Dac	envDac ( 8 );
	envDac.kinkedDac ( true );

	for ( auto i = 0; i < 256; ++i )
		voiceDC[ i ] = 5.0 * VOLTAGE_SKEW + ( drift * 0.2143 * envDac.getOutput ( i ) );
}
//-----------------------------------------------------------------------------

FilterModelConfig6581::FilterModelConfig6581 ()
	: FilterModelConfig (
		1.5,					// voice voltage range FIXME should theoretically be ~3,571V
		470e-12,				// capacitor value
		12.0 * VOLTAGE_SKEW,	// Vdd
		1.31,					// Vth
		20e-6,					// uCox
		opamp_voltage_6581,
		OPAMP_SIZE_6581
	)
	, WL_vcr ( 9.0 / 1.0 )
	, WL_snake ( 1.0 / 115.0 )
	, dac_zero ( 6.65 )
	, dac_scale ( 2.63 )
	, dac ( DAC_BITS )
{
	dac.kinkedDac ( true );

	setVoiceDCDrift ( 1.0 );

	// Create lookup tables for gains / summers
	auto clBuildSummerTable = [ this ]
	{
		OpAmp   opampModel ( std::vector<Spline::Point> ( std::begin ( opamp_voltage_6581 ), std::end ( opamp_voltage_6581 ) ), Vddt, vmin, vmax );
		buildSummerTable ( opampModel );
	};
	auto clBuildMixerTable = [ this ]
	{
		OpAmp   opampModel ( std::vector<Spline::Point> ( std::begin ( opamp_voltage_6581 ), std::end ( opamp_voltage_6581 ) ), Vddt, vmin, vmax );
		buildMixerTable ( opampModel, 8.0 / 6.0 );
	};
	auto clBuildVolumeTable = [ this ]
	{
		OpAmp   opampModel ( std::vector<Spline::Point> ( std::begin ( opamp_voltage_6581 ), std::end ( opamp_voltage_6581 ) ), Vddt, vmin, vmax );
		buildVolumeTable ( opampModel, 12.0 );
	};
	auto clBuildResonanceTable = [ this ]
	{
		OpAmp   opampModel ( std::vector<Spline::Point> ( std::begin ( opamp_voltage_6581 ), std::end ( opamp_voltage_6581 ) ), Vddt, vmin, vmax );

		// build temp n table
		double	resonance_n[ 16 ];
		for ( auto n8 = 0; n8 < 16; n8++ )
			resonance_n[ n8 ] = ( ~n8 & 0xF ) / 8.0;

		buildResonanceTable ( opampModel, resonance_n );
	};
	auto clFilterVcrVg = [ this ]
	{
		const auto  nVddt = N16 * ( Vddt - vmin );

		for ( auto i = 0u; i < ( 1 << 16 ); i++ )
		{
			// The table index is right-shifted 16 times in order to fit in
			// 16 bits; the argument to sqrt is thus multiplied by (1 << 16).
			const auto  tmp = nVddt - std::sqrt ( double ( i << 16 ) );
			assert ( tmp > -0.5 && tmp < 65535.5 );
			vcr_nVg[ i ] = uint16_t ( tmp + 0.5 );
		}
	};
	auto clFilterVcrIds = [ this ]
	{
		//  EKV model:
		//
		//  Ids = Is * (if - ir)
		//  Is = (2 * u*Cox * Ut^2)/k * W/L
		//  if = ln^2(1 + e^((k*(Vg - Vt) - Vs)/(2*Ut))
		//  ir = ln^2(1 + e^((k*(Vg - Vt) - Vd)/(2*Ut))

		// moderate inversion characteristic current
		const auto  Is = ( 2.0 * Ut * Ut ) * WL_vcr;

		// Normalized current factor for 1 cycle at 1MHz
		const auto  N15 = norm * ( ( 1 << 15 ) - 1 );
		const auto  n_Is = N15 * 1.0e-6 / C * Is;

		// kVgt_Vx = k*(Vg - Vt) - Vx
		// I.e. if k != 1.0, Vg must be scaled accordingly
		const auto	r_N16_2Ut = 1.0 / ( N16 * 2.0 * Ut );

		for ( auto i = 0; i < ( 1 << 16 ); i++ )
		{
			const auto	kVgt_Vx = i - ( 1 << 15 );
			const auto	log_term = std::log1p ( std::exp ( kVgt_Vx * r_N16_2Ut ) );
			// Scaled by m*2^15
			vcr_n_Ids_term[ i ] = n_Is * log_term * log_term;
		}
	};

	auto	thdSummer = std::thread ( clBuildSummerTable );
	auto	thdMixer = std::thread ( clBuildMixerTable );
	auto	thdVolume = std::thread ( clBuildVolumeTable );
	auto	thdResonance = std::thread ( clBuildResonanceTable );
	auto	thdFilterVcrVg = std::thread ( clFilterVcrVg );
	auto	thdFilterVcrIds = std::thread ( clFilterVcrIds );

	thdSummer.join ();
	thdMixer.join ();
	thdVolume.join ();
	thdResonance.join ();
	thdFilterVcrVg.join ();
	thdFilterVcrIds.join ();
}
//-----------------------------------------------------------------------------

uint16_t* FilterModelConfig6581::getDAC ( double adjustment ) const
{
	const auto  _dac_zero = getDacZero ( adjustment );

	auto    f0_dac = new uint16_t[ 1 << DAC_BITS ];

	for ( auto i = 0u; i < ( 1 << DAC_BITS ); i++ )
		f0_dac[ i ] = getNormalizedValue ( _dac_zero + dac.getOutput ( i ) * dac_scale );

	return f0_dac;
}
//-----------------------------------------------------------------------------

} // namespace reSIDfp
