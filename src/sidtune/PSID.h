#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2012-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <cstdint>
#include <vector>

#include "SidTuneBase.h"
#include "../sidplayfp/SidTune.h"

namespace libsidplayfp
{

constexpr auto	PSID_MAXSTRLEN = 32;

// Header has been extended for 'RSID' format
// The following changes are present:
//		id = 'RSID'
//		version = 2, 3 or 4
//		play, load and speed reserved 0
//		psidspecific flag is called C64BASIC flag
//		init cannot be under ROMS/IO memory area
//		load address cannot be less than $07E8
//		info strings may be 32 characters long without trailing zero
//		all values are big-endian
struct psidHeader
{
	uint32_t	id;						// 'PSID' or 'RSID' (ASCII)
	uint16_t	version;				// 1, 2, 3, or 4
	uint16_t	data;					// 16-bit offset to binary data in file
	uint16_t	load;					// 16-bit C64 address to load file to
	uint16_t	init;					// 16-bit C64 address of init subroutine
	uint16_t	play;					// 16-bit C64 address of play subroutine
	uint16_t	songs;					// number of songs
	uint16_t	start;					// start song out of [1..256]
	uint32_t	speed;					// bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)

	char	name[ PSID_MAXSTRLEN ];		// ASCII strings, 32 characters long and NOT terminated by a trailing zero
	char	author[ PSID_MAXSTRLEN ];
	char	released[ PSID_MAXSTRLEN ];

	uint16_t	flags;					// only version >= 2
	uint8_t		relocStartPage;			// only version >= 2ng
	uint8_t		relocPages;				// only version >= 2ng
	uint8_t		sidChipBase2;			// only version >= 3
	uint8_t		sidChipBase3;			// only version >= 4

	std::vector<uint16_t>	extraSids;	// one nSidFlags word per additional SID, 4E only
};
//-----------------------------------------------------------------------------

enum
{
	PSID_MUS = 1 << 0,
	PSID_SPECIFIC = 1 << 1, // These two are mutually exclusive
	PSID_BASIC = 1 << 1,
	PSID_CLOCK = 3 << 2,
	PSID_SIDMODEL = 3 << 4
};

enum
{
	PSID_CLOCK_UNKNOWN = 0,
	PSID_CLOCK_PAL = 1 << 2,
	PSID_CLOCK_NTSC = 1 << 3,
	PSID_CLOCK_ANY = PSID_CLOCK_PAL | PSID_CLOCK_NTSC
};

enum
{
	PSID_SIDMODEL_UNKNOWN = 0,
	PSID_SIDMODEL_6581 = 1,
	PSID_SIDMODEL_8580 = 2,
	PSID_SIDMODEL_ANY = PSID_SIDMODEL_6581 | PSID_SIDMODEL_8580
};

constexpr auto	psid_headerSize = 118;
constexpr auto	psidv2_headerSize = psid_headerSize + 6;

// WebSid's "SID file format+": a variable length list of SIDs with an output channel each,
// instead of v3/v4's hard coded second and third. See sid-format-4E.md
constexpr uint16_t	PSID_VERSION_4E = 0x004E;

// Output channel, at the same bit in the main flags word and in every nSidFlags word
constexpr auto	PSID_CHANNEL = 1 << 6;

// Where the nSidFlags list starts, on the word that v2 reserved
constexpr auto	psid4E_extraSidsOffset = psidv2_headerSize - 2;

// The chip address, as the two centre nibbles: 0x42 is $D420
constexpr auto	psid4E_addressShift = 8;

// Magic fields
constexpr	uint32_t PSID_ID = 0x50534944;
constexpr	uint32_t RSID_ID = 0x52534944;

//-----------------------------------------------------------------------------

class PSID final : public SidTuneBase
{
protected:
	PSID () = default;

public:
	/**
	* @return pointer to a SidTune or 0 if not a PSID file
	* @throw loadError if PSID file is corrupt
	*/
	[[ nodiscard ]] static SidTuneBase* load ( buffer_t& dataBuf );
	[[ nodiscard ]] const char* createMD5 ( char* md5 ) override;
	[[ nodiscard ]] const char* createMD5New ( char* md5 ) override;

private:
	char	m_md5[ SidTune::MD5_LENGTH + 1 ];

	/**
	* Load PSID file.
	*
	* @throw loadError
	*/
	void tryLoad ( const psidHeader& pHeader );

	/**
	* Read PSID file header.
	*
	* @throw loadError
	*/
	static void readHeader ( const buffer_t& dataBuf, psidHeader& hdr );

	// prevent copying
	PSID ( const PSID& ) = delete;
	PSID& operator=( PSID& ) = delete;
};
//-----------------------------------------------------------------------------

}
