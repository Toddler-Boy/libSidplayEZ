/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2012-2015 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2000-2001 Simon White
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

#include "PSID.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "../sidplayfp/SidTuneInfo.h"

#include "../MD5/MD5.h"

namespace libsidplayfp
{

//-----------------------------------------------------------------------------

/**
* Decode SID model flags
*/
SidTuneInfo::model_t getSidModel ( uint16_t modelFlag )
{
	if ( ( modelFlag & PSID_SIDMODEL_ANY ) == PSID_SIDMODEL_ANY )
		return SidTuneInfo::SIDMODEL_ANY;

	if ( modelFlag & PSID_SIDMODEL_6581 )
		return SidTuneInfo::SIDMODEL_6581;

	if ( modelFlag & PSID_SIDMODEL_8580 )
		return SidTuneInfo::SIDMODEL_8580;

	return SidTuneInfo::SIDMODEL_UNKNOWN;
}
//-----------------------------------------------------------------------------

SidTuneBase* PSID::load ( buffer_t& dataBuf )
{
	// File format check
	if ( dataBuf.size () < 4 )
		return nullptr;

	auto endian_big32 = [] ( const uint8_t ptr[ 4 ] ) { return uint32_t ( ( ptr[ 0 ] << 24 ) | ( ptr[ 1 ] << 16 ) | ( ptr[ 2 ] << 8 ) | ptr[ 3 ] ); };

	const auto	magic = endian_big32 ( &dataBuf[ 0 ] );
	if ( magic != PSID_ID && magic != RSID_ID )
		return nullptr;

	psidHeader	pHeader;
	readHeader ( dataBuf, pHeader );

	auto	tune = new PSID ();
	tune->tryLoad ( pHeader );

	return tune;
}
//-----------------------------------------------------------------------------

void PSID::readHeader ( const buffer_t& dataBuf, psidHeader& hdr )
{
	// Due to security concerns, input must be at least as long as version 1
	// header plus 16-bit C64 load address. That is the area which will be
	// accessed
	if ( dataBuf.size () < ( psid_headerSize + 2 ) )
		throw loadError ( ERR_TRUNCATED );

	auto get_big16 = [] ( const uint8_t ptr[ 2 ] ) -> uint16_t { return uint16_t ( ( ptr[ 0 ] << 8 ) | ptr[ 1 ] ); };
	auto endian_big32 = [] ( const uint8_t ptr[ 4 ] ) ->uint32_t { return uint32_t ( ( ptr[ 0 ] << 24 ) | ( ptr[ 1 ] << 16 ) | ( ptr[ 2 ] << 8 ) | ptr[ 3 ] ); };

	// Read v1 fields
	hdr.id = endian_big32 ( &dataBuf[ 0 ] );
	hdr.version = get_big16 ( &dataBuf[ 4 ] );
	hdr.data = get_big16 ( &dataBuf[ 6 ] );
	hdr.load = get_big16 ( &dataBuf[ 8 ] );
	hdr.init = get_big16 ( &dataBuf[ 10 ] );
	hdr.play = get_big16 ( &dataBuf[ 12 ] );
	hdr.songs = get_big16 ( &dataBuf[ 14 ] );
	hdr.start = get_big16 ( &dataBuf[ 16 ] );
	hdr.speed = endian_big32 ( &dataBuf[ 18 ] );
	memcpy ( hdr.name, &dataBuf[ 22 ], PSID_MAXSTRLEN );
	memcpy ( hdr.author, &dataBuf[ 54 ], PSID_MAXSTRLEN );
	memcpy ( hdr.released, &dataBuf[ 86 ], PSID_MAXSTRLEN );

	if ( hdr.version >= 2 )
	{
		if ( dataBuf.size () < ( psidv2_headerSize + 2 ) )
			throw loadError ( ERR_TRUNCATED );

		// Read v2/3/4 fields
		hdr.flags = get_big16 ( &dataBuf[ 118 ] );
		hdr.relocStartPage = dataBuf[ 120 ];
		hdr.relocPages = dataBuf[ 121 ];
		hdr.sidChipBase2 = dataBuf[ 122 ];
		hdr.sidChipBase3 = dataBuf[ 123 ];
	}
}
//-----------------------------------------------------------------------------

void PSID::tryLoad ( const psidHeader& pHeader )
{
	auto	compatibility = SidTuneInfo::COMPATIBILITY_C64;

	// Require a valid ID and version number.
	if ( pHeader.id == PSID_ID )
	{
		switch ( pHeader.version )
		{
			case 1:
				compatibility = SidTuneInfo::COMPATIBILITY_PSID;
				break;

			case 2:
			case 3:
			case 4:
				break;

			default:
				throw loadError ( "Unsupported PSID version" );
		}
		info.m_formatString = "PlaySID one-file format (PSID)";
	}
	else if ( pHeader.id == RSID_ID )
	{
		switch ( pHeader.version )
		{
			case 2:
			case 3:
			case 4:
				break;

			default:
				throw loadError ( "Unsupported RSID version" );
		}
		info.m_formatString = "Real C64 one-file format (RSID)";
		compatibility = SidTuneInfo::COMPATIBILITY_R64;
	}

	fileOffset = pHeader.data;
	info.m_loadAddr = pHeader.load;
	info.m_initAddr = pHeader.init;
	info.m_playAddr = pHeader.play;
	info.m_songs = pHeader.songs;
	info.m_startSong = pHeader.start;
	info.m_compatibility = compatibility;
	info.m_relocPages = 0;
	info.m_relocStartPage = 0;

	auto	speed = pHeader.speed;
	auto	clock = SidTuneInfo::CLOCK_UNKNOWN;

	auto	musPlayer = false;

	if ( pHeader.version >= 2 )
	{
		const auto	flags = pHeader.flags;

		// Check clock
		if ( flags & PSID_MUS )
		{
			// MUS tunes run at any speed
			clock = SidTuneInfo::CLOCK_ANY;
			musPlayer = true;
		}
		else
		{
			switch ( flags & PSID_CLOCK )
			{
				case PSID_CLOCK_ANY:			clock = SidTuneInfo::CLOCK_ANY;					break;
				case PSID_CLOCK_PAL:			clock = SidTuneInfo::CLOCK_PAL;					break;
				case PSID_CLOCK_NTSC:			clock = SidTuneInfo::CLOCK_NTSC;				break;

				default:	break;
			}
		}

		// These flags are only available for the appropriate file formats
		switch ( compatibility )
		{
			case SidTuneInfo::COMPATIBILITY_C64:
				if ( flags & PSID_SPECIFIC )
					info.m_compatibility = SidTuneInfo::COMPATIBILITY_PSID;
				break;

			case SidTuneInfo::COMPATIBILITY_R64:
				if ( flags & PSID_BASIC )
					info.m_compatibility = SidTuneInfo::COMPATIBILITY_BASIC;
				break;

			default:
				break;
		}

		info.m_clockSpeed = clock;

		info.m_sidModels[ 0 ] = getSidModel ( flags >> 4 );

		info.m_relocStartPage = pHeader.relocStartPage;
		info.m_relocPages = pHeader.relocPages;

		if ( pHeader.version >= 3 )
		{
			auto validateAddress = [] ( uint8_t address )
			{
				// Only even values are valid
				if ( address & 1 )
					return false;

				// Ranges $00-$41 ($D000-$D410) and $80-$DF ($D800-$DDF0) are invalid
				// Any invalid value means that no second SID is used, like $00
				if ( address <= 0x41 || ( address >= 0x80 && address <= 0xdf ) )
					return false;

				return true;
			};

			if ( validateAddress ( pHeader.sidChipBase2 ) )
			{
				info.m_sidChipAddresses.push_back ( 0xd000 | uint16_t ( pHeader.sidChipBase2 << 4 ) );
				info.m_sidModels.push_back ( getSidModel ( flags >> 6 ) );
			}

			if ( pHeader.version >= 4 )
			{
				if ( pHeader.sidChipBase3 != pHeader.sidChipBase2 && validateAddress ( pHeader.sidChipBase3 ) )
				{
					info.m_sidChipAddresses.push_back ( 0xd000 | uint16_t ( pHeader.sidChipBase3 << 4 ) );
					info.m_sidModels.push_back ( getSidModel ( flags >> 8 ) );
				}
			}
		}
	}

	// Check reserved fields to force real c64 compliance
	// as required by the RSID specification
	if ( compatibility == SidTuneInfo::COMPATIBILITY_R64 )
	{
		if ( info.m_loadAddr || info.m_playAddr || speed )
			throw loadError ( ERR_INVALID );

		// Real C64 tunes appear as CIA
		speed = ~0;
	}

	// Create the speed/clock setting table.
	convertOldStyleSpeedToTables ( speed, clock );

	// Copy info strings
	auto toStdString = [ this ] ( const char* infoStr )
	{
		char	tmp[ PSID_MAXSTRLEN + 1 ] = {};

		// The fields may use all 32 chars with no trailing zero
		for ( auto i = 0; i < PSID_MAXSTRLEN && infoStr[ i ]; i++ )
			tmp[ i ] = infoStr[ i ];

		info.m_infoString.emplace_back ( tmp );
	};

	toStdString ( pHeader.name );
	toStdString ( pHeader.author );
	toStdString ( pHeader.released );

	if ( musPlayer )
		throw loadError ( "Compute!'s Sidplayer MUS data is not supported yet" ); // TODO
}
//-----------------------------------------------------------------------------

const char* PSID::createMD5 ( char* md5 )
{
	if ( ! md5 )
		md5 = m_md5;

	*md5 = 0;

	// Include C64 data
	MD5	myMD5;
	myMD5.append ( &cache[ fileOffset ], info.m_c64dataLen );

	uint8_t tmp[ 2 ];

	auto set_little16 = [] ( uint8_t ptr[ 2 ], uint16_t word ) ->void { ptr[ 0 ] = uint8_t ( word );	ptr[ 1 ] = uint8_t ( word >> 8 ); };

	// Include INIT address
	set_little16 ( tmp, info.m_initAddr );
	myMD5.append ( tmp, sizeof ( tmp ) );

	// Include PLAY address
	set_little16 ( tmp, info.m_playAddr );
	myMD5.append ( tmp, sizeof ( tmp ) );

	// Include number of songs
	set_little16 ( tmp, uint16_t ( info.m_songs ) );
	myMD5.append ( tmp, sizeof ( tmp ) );

	{
		// Include song speed for each song
		const auto	currentSong = info.m_currentSong;

		for ( auto s = 1u; s <= info.m_songs; s++ )
		{
			selectSong ( s );
			const auto	songSpeed = uint8_t ( info.m_songSpeed );
			myMD5.append ( &songSpeed, sizeof ( songSpeed ) );
		}

		// Restore old song
		selectSong ( currentSong );
	}

	// Deal with PSID v2NG clock speed flags: Let only NTSC
	// clock speed change the MD5 fingerprint. That way the
	// fingerprint of a PAL-speed sidtune in PSID v1, v2, and
	// PSID v2NG format is the same
	if ( info.m_clockSpeed == SidTuneInfo::CLOCK_NTSC )
	{
		const uint8_t ntsc_val = 2;
		myMD5.append ( &ntsc_val, sizeof ( ntsc_val ) );
	}

	// NB! If the fingerprint is used as an index into a
	// song-lengths database or cache, modify above code to
	// allow for PSID v2NG files which have clock speed set to
	// SIDTUNE_CLOCK_ANY. If the SID player program fully
	// supports the SIDTUNE_CLOCK_ANY setting, a sidtune could
	// either create two different fingerprints depending on
	// the clock speed chosen by the player, or there could be
	// two different values stored in the database/cache
	myMD5.finish ();

	// Get fingerprint
	myMD5.getAscIIDigest ().copy ( md5, SidTune::MD5_LENGTH );
	md5[ SidTune::MD5_LENGTH ] = '\0';

	return md5;
}
//-----------------------------------------------------------------------------

const char* PSID::createMD5New ( char* md5 )
{
	if ( ! md5 )
		md5 = m_md5;

	*md5 = '\0';

	// The calculation is now simplified. All the header + all the data
	MD5	myMD5;
	myMD5.append ( &cache[ 0 ], int ( cache.size () ) );

	myMD5.finish ();

	// Get fingerprint
	myMD5.getAscIIDigest ().copy ( md5, SidTune::MD5_LENGTH );
	md5[ SidTune::MD5_LENGTH ] = '\0';

	return md5;
}
//-----------------------------------------------------------------------------

}
