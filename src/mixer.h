#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright (C) 2000 Simon White
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <cstdint>
#include <vector>

#include "EZ/config.h"

namespace libsidplayfp
{

class sidemu;

/**
* This class implements the mixer
*/
class Mixer
{
public:
	// Maximum number of supported SIDs
	static constexpr unsigned int MAX_SIDS = 3;

private:
	std::vector<sidemu*>	m_chips;

	// Mixer settings
	float*		m_sampleBuffer[ 2 ] = { nullptr, nullptr };
	int8_t**	m_digiBuffers = nullptr;
	uint32_t	m_sampleCount = 0;
	uint32_t	m_sampleIndex = 0;

	uint32_t	m_sampleRate = 0;
	bool		m_wait = false;

public:
	void doMix () noexcept;

	/**
	* This clocks the SID chips to the present moment, if they aren't already
	*/
	void clockChips () noexcept;

	/**
	* Reset sidemu buffer position discarding produced samples
	*/
	void resetBufs () noexcept;

	/**
	* Prepare for mixing cycle
	*
	* @param buffer output buffer
	* @param count size of the buffer in samples
	*/
	void begin ( float* bufferL, float* bufferR, int8_t** buffersDigi, uint32_t count ) noexcept;

	/**
	* Remove all SIDs from the mixer
	*/
	void clearSids () noexcept;

	/**
	* Add a SID to the mixer
	*
	* @param chip the sid emu to add
	*/
	void addSid ( sidemu* chip ) noexcept;

	/**
	* Get a SID from the mixer
	*
	* @param i the number of the SID to get
	* @return a pointer to the requested sid emu or 0 if not found
	*/
	[[ nodiscard ]] sidinline sidemu* getSid ( int i ) const noexcept { return ( i < int ( m_chips.size () ) ) ? m_chips[ i ] : nullptr; }

	/**
	* Set sample rate.
	*
	* @param rate sample rate in Hertz
	*/
	void setSamplerate ( uint32_t rate ) noexcept;

	/**
	* Check if the buffer have been filled
	*/
	[[ nodiscard ]] sidinline bool notFinished () const noexcept { return m_sampleIndex < m_sampleCount; }

	/**
	* Get the number of samples generated up to now
	*/
	[[ nodiscard ]] sidinline uint32_t samplesGenerated () const noexcept { return m_sampleIndex; }

	/*
	 * Wait till we consume the buffer
	 */
	[[ nodiscard ]] sidinline bool wait () const noexcept { return m_wait; }

	[[ nodiscard ]] sidinline int getNumChips () const noexcept { return int ( m_chips.size () ); }
};

}
