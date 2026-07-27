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

//-----------------------------------------------------------------------------

void Mixer::clockChips () noexcept
{
	for ( auto chp : m_chips )
		chp->clock ();
}
//-----------------------------------------------------------------------------

void Mixer::resetBufs () noexcept
{
	for ( auto chp : m_chips )
		chp->setBufferPos ( 0 );
}
//-----------------------------------------------------------------------------

void Mixer::doMix () noexcept
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

	// Copy digi buffers, unless the caller wants none
	if ( ! m_digiBuffers.empty () )
	{
		for ( auto i = 0; auto chp : m_chips )
		{
			const auto	buf = chp->getDigiBuffer ();

			// move digi data to final buffer
			std::memcpy ( m_digiBuffers[ i++ ].data () + m_sampleIndex, buf, toCopy );

			// move the unhandled data to start of buffer, if any (the ranges overlap)
			std::memmove ( buf, buf + toCopy, samplesLeft * sizeof ( *buf ) );
		}
	}

	// Render chips
	if ( ! m_sampleBuffer[ 1 ] )
	{
		//
		// Render all chips into same mono-output
		//
		for ( auto chp : m_chips )
		{
			const auto	buf = chp->getBuffer ();
			const auto	outBuf = m_sampleBuffer[ 0 ] + m_sampleIndex;

			if ( chp == m_chips[ 0 ] )
				convertBuffer ( outBuf, buf, toCopy );
			else
				sumBuffer ( outBuf, buf, toCopy );

			// move the unhandled data to start of buffer, if any
			std::memmove ( buf, buf + toCopy, samplesLeft * sizeof ( *buf ) );

			// Update sample-position
			chp->setBufferPos ( samplesLeft );
		}
	}
	else
	{
		//
		// Render every chip into the left buffer, the right one, or both
		//
		constexpr auto	centerGain = 0.708f;	// Center gain (-3dB)

		// The first chip on a side assigns, the rest sum
		bool	written[ 2 ] = { false, false };

		auto renderTo = [ & ] ( const int side, const int16_t* buf, const float gain )
		{
			const auto	outBuf = m_sampleBuffer[ side ] + m_sampleIndex;

			if ( written[ side ] )
			{
				for ( auto i = 0; i < toCopy; ++i )
					outBuf[ i ] += smp16ToFloat ( buf[ i ] ) * gain;
			}
			else
			{
				for ( auto i = 0; i < toCopy; ++i )
					outBuf[ i ] = smp16ToFloat ( buf[ i ] ) * gain;

				written[ side ] = true;
			}
		};

		for ( auto i = 0; auto chp : m_chips )
		{
			const auto	buf = chp->getBuffer ();

			if ( ! m_channels.empty () )
			{
				// The tune places its own chips
				const auto	side = ( i < int ( m_channels.size () ) && m_channels[ i ] ) ? 1 : 0;
				renderTo ( side, buf, 1.0f );
			}
			else if ( i == 0 )
			{
				renderTo ( 0, buf, 1.0f );
			}
			else if ( i == 1 )
			{
				renderTo ( 1, buf, 1.0f );
			}
			else
			{
				// The third chip and anything past it is centred
				renderTo ( 0, buf, centerGain );
				renderTo ( 1, buf, centerGain );
			}

			// move the unhandled data to start of buffer, if any
			std::memmove ( buf, buf + toCopy, samplesLeft * sizeof ( *buf ) );
			chp->setBufferPos ( samplesLeft );

			++i;
		}

		// Mirror into a side that got nothing
		for ( auto side = 0; side < 2; ++side )
			if ( ! written[ side ] && written[ ! side ] )
				std::copy_n ( m_sampleBuffer[ ! side ] + m_sampleIndex, toCopy, m_sampleBuffer[ side ] + m_sampleIndex );
	}

	m_sampleIndex += toCopy;
}
//-----------------------------------------------------------------------------

bool Mixer::needsMoreSamples () const noexcept
{
	// Clocking on top of samples the chips still hold would append past the end of their fixed buffer
	return m_chips.front ()->bufferpos () < int ( m_sampleCount - m_sampleIndex );
}
//-----------------------------------------------------------------------------

void Mixer::begin ( std::span<float> bufferL, std::span<float> bufferR, std::span<const std::span<int8_t>> digiBuffers ) noexcept
{
	// we need a minimum buffer-size, otherwise a crash might occur
	assert ( bufferL.size () > 100 );
	assert ( ( bufferR.empty () || bufferR.size () == bufferL.size () ) && "stereo needs two buffers of the same length" );

	m_sampleIndex = 0;
	m_sampleCount = uint32_t ( bufferL.size () );
	m_sampleBuffer[ 0 ] = bufferL.data ();
	m_sampleBuffer[ 1 ] = bufferR.empty () ? nullptr : bufferR.data ();
	m_digiBuffers = digiBuffers;

	// All or nothing: anything short of one full buffer per chip means the caller got its
	// chips wrong, so drop digi output rather than write past the end
	if ( ! m_digiBuffers.empty () )
	{
		auto	usable = m_digiBuffers.size () >= m_chips.size ();

		for ( auto i = 0u; usable && i < m_chips.size (); ++i )
			usable = m_digiBuffers[ i ].size () >= m_sampleCount;

		if ( ! usable )
		{
			assert ( false && "one digi buffer per chip is required, see getNumChips ()" );
			m_digiBuffers = {};
		}
	}
}
//-----------------------------------------------------------------------------

void Mixer::clearSids () noexcept
{
	m_chips.clear ();
	m_channels.clear ();
}
//-----------------------------------------------------------------------------

void Mixer::addSid ( sidemu* chip ) noexcept
{
	if ( chip )
		m_chips.push_back ( chip );
}
//-----------------------------------------------------------------------------

void Mixer::setChannels ( std::vector<uint8_t> channels ) noexcept
{
	m_channels = std::move ( channels );
}
//-----------------------------------------------------------------------------

void Mixer::setSamplerate ( uint32_t rate ) noexcept
{
	m_sampleRate = rate;
}
//-----------------------------------------------------------------------------

}
