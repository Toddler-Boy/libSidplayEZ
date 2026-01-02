#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2004 Dag Lem <resid@nimrod.no>
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

namespace reSIDfp
{
/**
* SID filter base class
*/
template<bool useFilter>
class Filter
{
protected:
	FilterModelConfig& fmc;

	uint16_t**	mixer = nullptr;
	uint16_t**	summer = nullptr;
	uint16_t*	resonance = nullptr;
	uint16_t*	volume = nullptr;

	// Current volume amplifier setting.
	uint16_t*	currentVolume = nullptr;

	// Current filter/voice mixer setting.
	uint16_t*	currentMixer = nullptr;

	// Filter input summer setting.
	uint16_t*	currentSummer = nullptr;

	// Filter resonance value.
	uint16_t*	currentResonance = nullptr;

	// Filter states (LP, BP, HP)
	int	Vlp = 0;
	int	Vbp = 0;
	int	Vhp = 0;

	// Filter external input
	int Ve = 0;

	// Filter cutoff frequency
	unsigned int fc = 0;

	// Switch voice 3 off
	int		voice3Mask = UINT_MAX;

	uint8_t	filterModeRouting = 0;		// bits = mute vce3, hp, bp, lp, fltE, flt3, flt2, flt1
	uint8_t	sumFltResults[ 256 ];		// Precalculate all possible summers and filter mixers

	/**
	* Set filter cutoff frequency.
	*/
	sidinline virtual void updatedCenterFrequency () = 0;

	/**
	* Mixing configuration modified (offsets change)
	*/
	sidinline void updateMixing ()
	{
		// Voice 3 is silenced by voice3off if it is not routed through the filter
		voice3Mask = ( filterModeRouting & 0x84 ) == 0x80 ? 0 : UINT_MAX;

		const auto	Nsum_Nmix = sumFltResults[ filterModeRouting ];

		currentMixer = mixer[ Nsum_Nmix & 0xF ];

		if constexpr ( useFilter )
			currentSummer = summer[ Nsum_Nmix >> 4 ];
	}

public:
	Filter ( FilterModelConfig& _fmc )
		: fmc ( _fmc )
	{
		// Pre-calculate all possible summer/mixer combinations
		for ( auto i = 0u; i < std::size ( sumFltResults ); ++i )
		{
			auto	Nsum = 0;
			auto	Nmix = 0;

			if ( i & 1 )	{ Nsum += 0x10; } else { Nmix++; }
	 		if ( i & 2 )	{ Nsum += 0x10; } else { Nmix++; }
	 		if ( i & 4 )	{ Nsum += 0x10; } else if ( ! ( i & 0x80 ) )	{ Nmix++; }
	 		if ( i & 8 )	{ Nsum += 0x10; } else { Nmix++; }

			if ( i & 0x10 )	Nmix++;
			if ( i & 0x20 )	Nmix++;
			if ( i & 0x40 )	Nmix++;

			sumFltResults[ i ] = uint8_t ( Nsum | Nmix );
		}

		mixer = fmc.getMixer ();
		summer = fmc.getSummer ();
		volume = fmc.getVolume ();
		resonance = fmc.getResonance ();
	}
	virtual ~Filter () = default;

	/**
	* SID reset.
	*/
	void reset ()
	{
		writeFC_LO ( 0 );
		writeFC_HI ( 0 );
		writeMODE_VOL ( 15 );
		writeRES_FILT ( 0 );
	}

	/**
	* Write Frequency Cutoff Low register.
	*
	* @param fc_lo Frequency Cutoff Low-Byte
	*/
	void writeFC_LO ( uint8_t fc_lo )
	{
		if constexpr ( useFilter )
		{
			fc = ( fc & 0x7F8 ) | ( fc_lo & 7 );

			updatedCenterFrequency ();
		}
	}

	/**
	* Write Frequency Cutoff High register.
	*
	* @param fc_hi Frequency Cutoff High-Byte
	*/
	void writeFC_HI ( uint8_t fc_hi )
	{
		if constexpr ( useFilter )
		{
			fc = ( ( fc_hi << 3 ) & 0x7F8 ) | ( fc & 7 );

			updatedCenterFrequency ();
		}
	}

	/**
	* Write Resonance/Filter register.
	*
	* @param res_filt Resonance/Filter
	*/
	void writeRES_FILT ( uint8_t res_filt )
	{
		constexpr auto	mask = useFilter ? 0x0F : 0x00;

		filterModeRouting = ( filterModeRouting & 0xF0 ) | ( res_filt & mask );

		if constexpr ( useFilter )
			currentResonance = resonance + ( ( res_filt >> 4 ) << 16 );

		updateMixing ();
	}

	/**
	* Write filter Mode/Volume register.
	*
	* @param mode_vol Filter Mode/Volume
	*/
	void writeMODE_VOL ( uint8_t mode_vol )
	{
		constexpr auto	mask = useFilter ? 0xF0 : 0x80;

		filterModeRouting = ( filterModeRouting & 0x0F ) | ( mode_vol & mask );

		currentVolume = volume + ( ( mode_vol & 0x0F ) << 16 );

		updateMixing ();
	}
};
//-----------------------------------------------------------------------------

} // namespace reSIDfp
