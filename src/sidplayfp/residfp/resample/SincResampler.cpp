/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2023 Michael Hartmann
 * Copyright 2011-2020 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "SincResampler.h"

#include <cassert>
#include <cmath>
#include <numeric>
#include <execution>

#if ! defined __APPLE__
constexpr auto	M_PI = 3.14159265358979323846;
#endif
//-----------------------------------------------------------------------------

namespace reSIDfp
{

void SincResampler::setup ( double clockFrequency, double samplingFrequency, double highestAccurateFrequency )
{
	reset ();

	cyclesPerSample = int ( clockFrequency / samplingFrequency * 1024.0 );

	// 16 bits -> -96dB stopband attenuation.
	constexpr auto	BITS = 16;
	const auto	A = -20.0 * std::log10 ( 1.0 / ( 1 << BITS ) );

	// A fraction of the bandwidth is allocated to the transition band, which we double because we design the filter to transition halfway at Nyquist
	const auto	dw = ( 1.0 - 2.0 * highestAccurateFrequency / samplingFrequency ) * M_PI * 2.0;

	constexpr auto I0 = [] ( double x )
	{
		// Maximum error acceptable in I0 is 1e-6, or ~96 dB
		constexpr auto	I0E = 1e-6;

		/**
		* Compute the 0th order modified Bessel function of the first kind.
		* This function is originally from resample-1.5/filterkit.c by J. O. Smith.
		* It is used to build the Kaiser window for resampling.
		*
		* @param x evaluate I0 at x
		* @return value of I0 at x.
		*/
		auto	sum = 1.0;
		auto	u = 1.0;
		auto	n = 1.0;
		const auto	halfx = x / 2.0;

		do
		{
			const auto	temp = halfx / n;

			u *= temp * temp;
			sum += u;
			n += 1.0;

		} while ( u >= I0E * sum );

		return sum;
	};

	// For calculation of beta and N see the reference for the Kaiser window function in the MATLAB Signal Processing Toolbox:
	// http://www.mathworks.com/help/signal/ref/kaiserord.html
	const auto	beta = 0.1102 * ( A - 8.7 );
	const auto	I0beta = I0 ( beta );
	const auto	cyclesPerSampleD = clockFrequency / samplingFrequency;
	const auto	r_cyclesPerSampleD = 1.0 / cyclesPerSampleD;

	{
		// The filter order will maximally be 124 with the current constraints.
		// N >= (96.33 - 7.95)/(2 * pi * 2.285 * (maxfreq - passbandfreq) >= 123
		// The filter order is equal to the number of zero crossings, i.e.
		// it should be an even number (sinc is symmetric with respect to x = 0).
		auto	N = int ( ( A - 7.95 ) / ( 2.285 * dw ) + 0.5 );
		N += N & 1;

		// The filter length is equal to the filter order + 1.
		// The filter length must be an odd number (sinc is symmetric with respect to
		// x = 0).
		firN = int ( ( N * cyclesPerSampleD ) + 1 ) | 1;

		// Check whether the sample ring buffer would overflow.
		assert ( firN < RINGSIZE );

		// Error is bounded by err < 1.234 / L^2, so L = sqrt(1.234 / (2^-16)) = sqrt(1.234 * 2^16).
		firRES = int ( std::ceil ( std::sqrt ( 1.234 * ( 1 << BITS ) ) * r_cyclesPerSampleD ) );

		// firN*firRES represent the total resolution of the sinc sampling. JOS
		// recommends a length of 2^BITS, but we don't quite use that good a filter.
		// The filter test program indicates that the filter performs well, though.
	}

	// Allocate memory for FIR table
	firTable.resize ( firRES * firN );

	// The cutoff frequency is midway through the transition band, in effect the same as Nyquist
	constexpr auto	wc = M_PI;

	// Calculate the sinc tables
	const auto	scale = 32768.0 * wc * r_cyclesPerSampleD / M_PI;

	// We're not interested in the fractional part so use int division before converting to double
	const auto	tmp = firN / 2;
	const auto	firN_2 = double ( tmp );

	auto	dst = firTable.data ();
	for ( auto i = 0; i < firRES; i++ )
	{
		const auto	jPhase = double ( i ) / firRES + firN_2;

		for ( auto j = 0; j < firN; j++ )
		{
			const auto	x = j - jPhase;

			const auto	xt = x / firN_2;
			const auto	kaiserXt = std::fabs ( xt ) < 1.0 ? I0 ( beta * std::sqrt ( 1. - xt * xt ) ) / I0beta : 0.0;

			const auto	wt = wc * x * r_cyclesPerSampleD;
			const auto	sincWt = std::fabs ( wt ) >= 1e-8 ? std::sin ( wt ) / wt : 1.0;

			*dst++ = int16_t ( scale * sincWt * kaiserXt );
		}
	}
}
//-----------------------------------------------------------------------------

void SincResampler::reset ()
{
	std::fill ( std::begin ( sample ), std::end ( sample ), 0 );
	sampleOffset = 0;
}
//-----------------------------------------------------------------------------

} // namespace reSIDfp
