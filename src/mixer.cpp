/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2000 Simon White
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

#include <algorithm>
#include <cassert>
#include <cstring>

#include "mixer.h"
#include "sidemu.h"

namespace libsidplayfp {

void Mixer::clockChips ()
{
	for ( auto chp : m_chips )
		chp->clock ();
}
//-----------------------------------------------------------------------------

void Mixer::resetBufs ()
{
	for ( auto chp : m_chips )
		chp->bufferpos ( 0 );
}
//-----------------------------------------------------------------------------

void Mixer::doMix ()
{
	// extract buffer info now that the SID is updated
	// clock() may update bufferpos
	// NB: if more than one chip exists, their bufferpos is identical to first chip's
	const auto	sampleCount = m_chips.front ()->bufferpos ();
	const auto	toCopy = std::min ( sampleCount, int ( m_sampleCount - m_sampleIndex ) );

	const auto	samplesLeft = sampleCount - toCopy;

	constexpr auto smp16ToFloat = [] ( const int16_t input )
	{
		constexpr auto inv = 1.0f / ( INT16_MAX + 1 );

		return input * inv;
	};

	auto convertBuffer = [ &smp16ToFloat ] ( float* out, int16_t* in, int count )
	{
		for ( auto i = 0; i < count; ++i )
			out[ i ] = smp16ToFloat ( in[ i ] );
	};

	auto sumBuffer = [ &smp16ToFloat ] ( float* out, int16_t* in, int count )
	{
		for ( auto i = 0; i < count; ++i )
			out[ i ] += smp16ToFloat ( in[ i ] );
	};

	// Render chips
	if ( ! m_sampleBuffer[ 1 ] )
	{
		//
		// Render all chips into same mono-output
		//
		for ( auto chp : m_chips )
		{
			const auto	buf = chp->buffer ();
			const auto	outBuf = m_sampleBuffer[ 0 ] + m_sampleIndex;

			if ( chp == m_chips[ 0 ] )
				convertBuffer ( outBuf, buf, toCopy );
			else
				sumBuffer ( outBuf, buf, toCopy );

			// move the unhandled data to start of buffer, if any
			std::memmove ( buf, buf + toCopy, samplesLeft * sizeof ( *buf ) );
			chp->bufferpos ( samplesLeft );
		}
	}
	else
	{
		//
		// Render first two chips into separate output buffers
		//
		const auto	numChips = int ( m_chips.size () );
		for ( auto i = 0; i < std::min ( 2, numChips ); ++i )
		{
			auto		chp = m_chips[ i ];
			const auto	buf = chp->buffer ();
			const auto	outBuf = m_sampleBuffer[ i ] + m_sampleIndex;

			convertBuffer ( outBuf, buf, toCopy );

			// move the unhandled data to start of buffer, if any
			std::memmove ( buf, buf + toCopy, samplesLeft * sizeof ( *buf ) );
			chp->bufferpos ( samplesLeft );
		}

		if ( numChips == 1 )
			std::copy_n ( m_sampleBuffer[ 0 ] + m_sampleIndex, toCopy, m_sampleBuffer[ 1 ] + m_sampleIndex );

		if ( m_chips.size () == 3 )
		{
			auto		chp = m_chips[ 2 ];
			const auto	buf = chp->buffer ();
			const auto	outBufL = m_sampleBuffer[ 0 ] + m_sampleIndex;
			const auto	outBufR = m_sampleBuffer[ 1 ] + m_sampleIndex;

			constexpr auto	centerGain = 0.708f;	// Center gain (-3dB)

			for ( auto i = 0; i < toCopy; ++i )
			{
				const auto	center = smp16ToFloat ( buf[ i ] ) * centerGain;
				outBufL[ i ] += center;
				outBufR[ i ] += center;
			}

			// move the unhandled data to start of buffer, if any
			std::memmove ( buf, buf + toCopy, samplesLeft * sizeof ( *buf ) );
			chp->bufferpos ( samplesLeft );
		}
	}

	m_wait = uint32_t ( samplesLeft ) > m_sampleCount;
	m_sampleIndex += toCopy;
}
//-----------------------------------------------------------------------------

void Mixer::begin ( float* bufferL, float* bufferR, uint32_t count )
{
	// we need a minimum buffer-size, otherwise a crash might occur
	assert ( count > 100 );

	m_sampleIndex = 0;
	m_sampleCount = count;
	m_sampleBuffer[ 0 ] = bufferL;
	m_sampleBuffer[ 1 ] = bufferR;

	m_wait = false;
}
//-----------------------------------------------------------------------------

void Mixer::clearSids ()
{
	m_chips.clear ();
}
//-----------------------------------------------------------------------------

void Mixer::addSid ( sidemu* chip )
{
	if ( chip )
		m_chips.push_back ( chip );
}
//-----------------------------------------------------------------------------

void Mixer::setSamplerate ( uint32_t rate )
{
	m_sampleRate = rate;
}
//-----------------------------------------------------------------------------

}
