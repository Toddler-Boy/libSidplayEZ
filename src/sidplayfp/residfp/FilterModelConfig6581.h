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

	void clFilterVcrIds () noexcept;
	[[ nodiscard ]] sidinline double getDacZero ( double adjustment ) const noexcept {	return dac_zero + adjustment;	}

	// Voice DC offset LUT
	double	voiceDC[ 256 ];

public:
	FilterModelConfig6581 ();

	void setFilterRange ( double adjustment ) noexcept;

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
