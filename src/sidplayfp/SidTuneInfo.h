#pragma once
/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2011-2017 Leandro Nini
 *  Copyright 2007-2010 Antti Lankila
 *  Copyright 2000 Simon White
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <cstdint>

/**
* This interface is used to get values from SidTune objects.
*
* You must read (i.e. activate) sub-song specific information
* via:
*        const SidTuneInfo* tuneInfo = SidTune.getInfo();
*        const SidTuneInfo* tuneInfo = SidTune.getInfo(songNumber);
*/
class SidTuneInfo
{
public:
	typedef enum {
		CLOCK_UNKNOWN,
		CLOCK_PAL,
		CLOCK_NTSC,
		CLOCK_ANY
	} clock_t;

	typedef enum {
		SIDMODEL_UNKNOWN,
		SIDMODEL_6581,
		SIDMODEL_8580,
		SIDMODEL_ANY
	} model_t;

	typedef enum {
		COMPATIBILITY_C64,		// File is C64 compatible
		COMPATIBILITY_PSID,		// File is PSID specific
		COMPATIBILITY_R64,		// File is Real C64 only
		COMPATIBILITY_BASIC		// File requires C64 Basic
	} compatibility_t;

	static constexpr int SPEED_VBI = 0;		// Vertical-Blanking-Interrupt
	static constexpr int SPEED_CIA_1A = 60;	// CIA 1 Timer A

	[[ nodiscard ]] uint16_t loadAddr () const { return getLoadAddr (); }
	[[ nodiscard ]] uint16_t initAddr () const { return getInitAddr (); }
	[[ nodiscard ]] uint16_t playAddr () const { return getPlayAddr (); }

	[[ nodiscard ]] unsigned int songs () const { return getSongs (); }				// Number of songs
	[[ nodiscard ]] unsigned int startSong () const { return getStartSong (); }		// The default starting song
	[[ nodiscard ]] unsigned int currentSong () const { return getCurrentSong (); }

	/**
	 * @name Base addresses
	 * The SID chip base address(es) used by the sidtune.
	 * - 0xD400 for the 1st SID
	 * - 0 if the nth SID is not required
	 */
	[[ nodiscard ]] uint16_t sidChipBase ( unsigned int i ) const { return getSidChipBase ( i ); }
	[[ nodiscard ]] int sidChips () const { return getSidChips (); }		// The number of SID chips required by the tune

	[[ nodiscard ]] int songSpeed () const { return getSongSpeed (); }

	[[ nodiscard ]] uint8_t relocStartPage () const { return getRelocStartPage (); }	// First available page for relocation
	[[ nodiscard ]] uint8_t relocPages () const { return getRelocPages (); }			// Number of pages available for relocation

	[[ nodiscard ]] SidTuneInfo::model_t sidModel ( unsigned int i ) const { return getSidModel ( i ); }	// The SID chip model(s) requested by the sidtune
	[[ nodiscard ]] SidTuneInfo::compatibility_t compatibility () const { return getCompatibility (); }		// Compatibility requirements

	/**
	 * Song title, credits, ...
	 * - 0 = Title
	 * - 1 = Author
	 * - 2 = Released
	 */
	[[ nodiscard ]] unsigned int numberOfInfoStrings () const { return getNumberOfInfoStrings (); }
	[[ nodiscard ]] const char* infoString ( unsigned int i ) const { return getInfoString ( i ); }

	//
	// MUS comments
	//
	[[ nodiscard ]] unsigned int numberOfCommentStrings () const { return getNumberOfCommentStrings (); }
	[[ nodiscard ]] const char* commentString ( unsigned int i ) const { return getCommentString ( i ); }

	[[ nodiscard ]] uint32_t dataFileLen () const { return getDataFileLen (); }		// Length of single-file sidtune file
	[[ nodiscard ]] uint32_t c64dataLen () const { return getC64dataLen (); }		// Length of raw C64 data without load address

	[[ nodiscard ]] SidTuneInfo::clock_t clockSpeed () const { return getClockSpeed (); }	// The tune clock speed

	[[ nodiscard ]] const char* formatString () const { return getFormatString (); }		// The name of the identified file format

	[[ nodiscard ]] bool fixLoad () const { return getFixLoad (); }					// Whether load address might be duplicate

	[[ nodiscard ]] const char* path () const { return getPath (); }					// Path to sidtune file
	[[ nodiscard ]] const char* dataFileName () const { return getDataFileName (); }	// A first file: e.g. "foo.sid" or "foo.mus"
	/**
	 * A second file: e.g. "foo.str".
	 * Returns 0 if none.
	 */
	[[ nodiscard ]] const char* infoFileName () const { return getInfoFileName (); }

private:
	[[ nodiscard ]] virtual uint16_t getLoadAddr () const = 0;
	[[ nodiscard ]] virtual uint16_t getInitAddr () const = 0;
	[[ nodiscard ]] virtual uint16_t getPlayAddr () const = 0;

	[[ nodiscard ]] virtual unsigned int getSongs () const = 0;
	[[ nodiscard ]] virtual unsigned int getStartSong () const = 0;
	[[ nodiscard ]] virtual unsigned int getCurrentSong () const = 0;

	[[ nodiscard ]] virtual uint16_t getSidChipBase ( unsigned int i ) const = 0;
	[[ nodiscard ]] virtual int getSidChips () const = 0;

	[[ nodiscard ]] virtual int getSongSpeed () const = 0;

	[[ nodiscard ]] virtual uint8_t getRelocStartPage () const = 0;
	[[ nodiscard ]] virtual uint8_t getRelocPages () const = 0;

	[[ nodiscard ]] virtual model_t getSidModel ( unsigned int i ) const = 0;
	[[ nodiscard ]] virtual compatibility_t getCompatibility () const = 0;

	[[ nodiscard ]] virtual unsigned int getNumberOfInfoStrings () const = 0;
	[[ nodiscard ]] virtual const char* getInfoString ( unsigned int i ) const = 0;
	
	[[ nodiscard ]] virtual unsigned int getNumberOfCommentStrings () const = 0;
	[[ nodiscard ]] virtual const char* getCommentString ( unsigned int i ) const = 0;

	[[ nodiscard ]] virtual uint32_t getDataFileLen () const = 0;
	[[ nodiscard ]] virtual uint32_t getC64dataLen () const = 0;

	[[ nodiscard ]] virtual clock_t getClockSpeed () const = 0;

	[[ nodiscard ]] virtual const char* getFormatString () const = 0;

	[[ nodiscard ]] virtual bool getFixLoad () const = 0;

	[[ nodiscard ]] virtual const char* getPath () const = 0;
	[[ nodiscard ]] virtual const char* getDataFileName () const = 0;
	[[ nodiscard ]] virtual const char* getInfoFileName () const = 0;
};
