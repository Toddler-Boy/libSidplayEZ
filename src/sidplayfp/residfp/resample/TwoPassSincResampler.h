#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2023 Michael Hartmann
* Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
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

#include <cmath>

#include "SincResampler.h"

namespace reSIDfp
{
/**
* Compose a more efficient SINC from chaining two other SINCs
*/
template <int scaleFactor>
class TwoPassSincResampler final
{
public:
	void setup ( double clockFrequency, double samplingFrequency )
	{
		// Set the passband frequency slightly below half sampling frequency
		//   pass_freq <= 0.9*sample_freq/2
		//
		// This constraint ensures that the FIR table is not overfilled.
		// For higher sampling frequencies we're fine with 20KHz
		const auto	halfFreq = ( samplingFrequency > 44000.0 ) ? 20000.0 : samplingFrequency * 0.45;

		// Calculation according to Laurent Ganier.
		// It evaluates to about 120 kHz at typical settings.
		// Some testing around the chosen value seems to confirm that this does work.
		const auto	intermediateFrequency = 2.0 * halfFreq
			+	std::sqrt	( 2.0 * halfFreq * clockFrequency
								* ( samplingFrequency - 2.0 * halfFreq ) / samplingFrequency
							);

		s1.setup ( clockFrequency, intermediateFrequency, halfFreq );
		s2.setup ( intermediateFrequency, samplingFrequency, halfFreq );
	}

	[[ nodiscard ]] sidinline bool input ( const int sample )
	{
		return s1.input ( sample ) && s2.input ( s1.output () );
	}

	[[ nodiscard ]] sidinline int16_t output () const
	{
		/*
		* Clip the input as it may overflow the 16 bit range.
		*
		* Approximate measured input ranges:
		* 6581: [-24262,+25080]  (Kawasaki_Synthesizer_Demo)
		* 8580: [-21514,+35232]  (64_Forever, Drum_Fool)
		*/
		auto softClip = [] ( const int x ) -> int16_t
		{
			constexpr auto	max16 = float ( std::numeric_limits<int16_t>::max () );
			constexpr auto	threshold = int ( max16 * 0.85454f );
			const auto	abs_x = std::abs ( x );
			if ( abs_x < threshold ) [[ likely ]]
				return int16_t ( x );

			constexpr auto	t = threshold / max16;
			constexpr auto	a = 1.0f - t;
			constexpr auto	b = 1.0f / a;

			auto	value = float ( abs_x - threshold ) / max16;
			value = t + a * std::tanh ( b * value );

			return int16_t ( value * ( x < 0 ? -max16 : max16 ) );
		};

		const auto	out = ( scaleFactor * s2.output () ) >> 1;
		return softClip ( out );
	}

	void reset ()
	{
		s1.reset ();
		s2.reset ();
	}

private:
	SincResampler	s1;
	SincResampler	s2;
};

} // namespace reSIDfp
