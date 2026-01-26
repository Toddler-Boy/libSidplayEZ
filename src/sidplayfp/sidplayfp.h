#pragma once
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

#include <cstdint>

#include "player.h"

struct SidConfig;
class  SidTune;
class  SidInfo;
class  EventContext;

/**
 * sidplayfp
 */
class sidplayfp
{
private:
	libsidplayfp::Player	sidplayer;

public:
	/**
	 * Get the current engine configuration.
	 *
	 * @return a const reference to the current configuration.
	 */
	[[ nodiscard ]] const SidConfig& config () const {	return sidplayer.config ();	}

	/**
	 * Get the current player informations.
	 *
	 * @return a const reference to the current info.
	 */
	[[ nodiscard ]] const SidInfo& info () const {	return sidplayer.info ();	}

	/**
	 * Configure the engine.
	 * Check #error for detailed message if something goes wrong.
	 *
	 * @param cfg the new configuration
	 * @return true on success, false otherwise.
	 */
	bool config ( const SidConfig& cfg ) {	return sidplayer.config ( cfg );	}

	/**
	 * Error message.
	 *
	 * @return string error message.
	 */
	[[ nodiscard ]] const char* error () const { return sidplayer.error (); }

	/**
	 * Load a tune.
	 * Check #error for detailed message if something goes wrong.
	 *
	 * @param tune the SidTune to load, 0 unloads current tune.
	 * @return true on sucess, false otherwise.
	 */
	bool load ( SidTune* tune )	{	return sidplayer.load(tune);	}

	/**
	 * Run the emulation and produce samples to play if a buffer is given.
	 *
	 * @param buffer pointer to the buffer to fill with samples.
	 * @param count the size of the buffer measured in 16 bit samples
	 * @return the number of produced samples. If less than requested
	 *         or #isPlaying() is false an error occurred, use #error()
	 *         to get a detailed message.
	 */
	uint32_t play ( int16_t* buffer, uint32_t count ) {	return sidplayer.play ( buffer, count );	}

	/**
	 * Get the current playing time.
	 *
	 * @return the current playing time measured in seconds.
	 */
	[[ nodiscard ]] uint32_t time () const { return sidplayer.timeMs () / 1000; }

	/**
	 * Get the current playing time.
	 *
	 * @return the current playing time measured in milliseconds.
	 * @since 2.0
	 */
	[[ nodiscard ]] uint32_t timeMs () const { return sidplayer.timeMs (); }

	/**
	 * Set ROM images.
	 *
	 * @param kernal pointer to Kernal ROM.
	 * @param basic pointer to Basic ROM, generally needed only for BASIC tunes.
	 * @param character pointer to character generator ROM.
	 */
	void setRoms ( const uint8_t* kernal, const uint8_t* basic = 0, const uint8_t* character = 0 )
	{
		setKernal ( kernal );
		setBasic ( basic );
		setChargen ( character );
	}

	/**
	 * Set the ROM banks.
	 *
	 * @param rom pointer to the ROM data.
	 * @since 2.2
	 */
	 //@{
	void setKernal ( const uint8_t* rom )	{	sidplayer.setKernal ( rom );	}
	void setBasic ( const uint8_t* rom )	{	sidplayer.setBasic ( rom );		}
	void setChargen ( const uint8_t* rom )	{	sidplayer.setChargen ( rom );	}
	//@}

	/**
	 * Get the CIA 1 Timer A programmed value.
	 */
	uint16_t getCia1TimerA () const	{	return sidplayer.getCia1TimerA ();	}

	/**
	 * Get the SID registers programmed value.
	 *
	 * @param sidNum the SID chip, 0 for the first one, 1 for the second and 2 for the third.
	 * @param regs an array that will be filled with the last values written to the chip.
	 * @return false if the requested chip doesn't exist.
	 * @since 2.2
	 */
	bool getSidStatus ( int sidNum, uint8_t regs[ 32 ] ) {	return sidplayer.getSidStatus ( sidNum, regs );	}
};
//-----------------------------------------------------------------------------
