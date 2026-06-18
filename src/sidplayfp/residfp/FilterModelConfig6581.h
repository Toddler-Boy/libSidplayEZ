#pragma once
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
#include "Dac.h"

namespace reSIDfp
{

/**
* Calculate parameters for 6581 filter emulation.
*/
class FilterModelConfig6581 final : public FilterModelConfig
{
private:
	/**
	* Power bricks generate voltages slightly out of spec
	*/
	static constexpr auto	VOLTAGE_SKEW = 1.015;

	// Transistor parameters.
	//@{
	const double WL_vcr;        ///< W/L for VCR
	const double WL_snake;      ///< W/L for "snake"
	//@}

	// DAC parameters.
	//@{
	const double dac_zero;
	const double dac_scale;
	//@}

	// DAC lookup table
	Dac	dac;

	// VCR - 6581 only.
	//@{
	uint16_t	vcr_nVg[ 1 << 16 ];
	uint16_t	vcr_n_Ids_term[ 1 << 16 ];
	//@}

	double	vcrSaturation = 1.0;	// 0.0 = linear VCR, 1.0 = full EKV nonlinearity

	void clFilterVcrIds () noexcept;
	[[ nodiscard ]] sidinline double getDacZero ( double adjustment ) const noexcept {	return dac_zero + adjustment;	}

	// Voice DC offset LUT
	double	voiceDC[ 256 ];
	int		normalizedVoiceDC[ 256 ];	// getNormalizedVoice(0.0f, env) for each envelope value

public:
	FilterModelConfig6581 ();

	// Set support-hardware for the filter model.
	// This is used to adjust the model for different hardware revisions of the C64
	void setFilter_uCoxAndCap ( double newUCox, bool oldCap ) noexcept;

	/**
	* Set VCR saturation amount. Blends vcr_n_Ids_term between the full EKV
	* log²(1+eˣ) curve (1.0) and a linear approximation tangent at x=0 (0.0).
	* Rebuilds the table; not intended for per-sample use.
	*/
	void setVcrSaturation ( double saturation ) noexcept;

	void setVoiceDCDrift ( double drift ) noexcept;

	/**
	* Construct an 11 bit cutoff frequency DAC output voltage table.
	* Ownership is transferred to the requester which becomes responsible
	* of freeing the object when done.
	*
	* @param adjustment
	* @return the DAC table
	*/
	[[ nodiscard ]] uint16_t* getDAC ( double adjustment ) const noexcept;
	[[ nodiscard ]] double getWL_snake () const noexcept { return WL_snake; }

	[[ nodiscard ]] sidinline uint16_t getVcr_nVg ( const int i ) const noexcept {	return vcr_nVg[ i ]; }
	[[ nodiscard ]] sidinline unsigned int getVcr_n_Ids_term ( const int i ) const noexcept {	return vcr_n_Ids_term[ i ]; }
	[[ nodiscard ]] sidinline const uint16_t* getVcr_nVgPtr () const noexcept { return vcr_nVg; }
	[[ nodiscard ]] sidinline const uint16_t* getVcr_n_Ids_termPtr () const noexcept { return vcr_n_Ids_term; }

	[[ nodiscard ]] sidinline int getNormalizedVoiceDC ( unsigned int env ) const noexcept { return normalizedVoiceDC[ env ]; }

	[[ nodiscard ]] sidinline int getNormalizedVoice ( float value, unsigned int env ) const noexcept
	{
		const auto	tmp = N16 * ( ( value * voice_voltage_range + voiceDC[ env ] ) - vmin );

		assert ( tmp >= 0.0 && tmp < 65536.0 );
		return int ( tmp );
	}

	#if 0
		[[ nodiscard ]] sidinline double getUt () const { return Ut; }
		[[ nodiscard ]] sidinline double getN16 () const { return N16; }
	#endif
};

} // namespace reSIDfp
