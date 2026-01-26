/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "p00.h"

#include <cstdint>
#include <cstring>
#include <cctype>

#include "../sidplayfp/SidTuneInfo.h"

#include "SidTuneTools.h"

namespace libsidplayfp
{

constexpr auto	X00_ID_LEN = 8;
constexpr auto	X00_NAME_LEN = 17;

// File format from PC64. PC64 automatically generates
// the filename from the cbm name (16 to 8 conversion)
// but we only need to worry about that when writing files
// should we want pc64 compatibility.  The extension numbers
// are just an index to try to avoid repeats.  Name conversion
// works by creating an initial filename from alphanumeric
// and ' ', '-' characters only with the later two being
// converted to '_'.  Then it parses the filename
// from end to start removing characters stopping as soon
// as the filename becomes <= 8.  The removal of characters
// occurs in three passes, the first removes all '_', then
// vowels and finally numerics.  If the filename is still
// greater than 8 it is truncated.

struct X00Header
{
	char    id[ X00_ID_LEN ];     // 'C64File' (ASCII)
	uint8_t name[ X00_NAME_LEN ]; // C64 name (PETSCII)
	uint8_t length;             // Rel files only (Bytes/Record),
	// should be 0 for all other types
};

enum class X00Format
{
	X00_DEL,
	X00_SEQ,
	X00_PRG,
	X00_USR,
	X00_REL
};

//-----------------------------------------------------------------------------

SidTuneBase* p00::load ( const char* fileName, buffer_t& dataBuf )
{
	auto	ext = SidTuneTools::fileExtOfPath ( fileName );

	// Combined extension & magic field identification
	if ( strlen ( ext ) != 4 )
		return nullptr;

	if ( ! std::isdigit ( ext[ 2 ] ) || ! std::isdigit ( ext[ 3 ] ) )
		return nullptr;

	const char*	format = nullptr;
	X00Format	type;

	switch ( std::toupper ( ext[ 1 ] ) )
	{
		case 'D':		type = X00Format::X00_DEL;		format = "Unsupported tape image file (DEL)";		break;
		case 'S':		type = X00Format::X00_SEQ;		format = "Unsupported tape image file (SEQ)";		break;
		case 'P':		type = X00Format::X00_PRG;		format = "Tape image file (PRG)";					break;
		case 'U':		type = X00Format::X00_USR;		format = "Unsupported USR file (USR)";				break;
		case 'R':		type = X00Format::X00_REL;		format = "Unsupported tape image file (REL)";		break;

		default:		return nullptr;
	}

	// Verify the file is what we think it is
	const auto	bufLen = dataBuf.size ();
	if ( bufLen < X00_ID_LEN )
		return nullptr;

	X00Header pHeader;
	memcpy ( pHeader.id, &dataBuf[ 0 ], X00_ID_LEN );
	memcpy ( pHeader.name, &dataBuf[ X00_ID_LEN ], X00_NAME_LEN );
	pHeader.length = dataBuf[ X00_ID_LEN + X00_NAME_LEN ];

	// Magic field
	if ( strcmp ( pHeader.id, "C64File" ) )
		return nullptr;

	// File types current supported
	if ( type != X00Format::X00_PRG )
		throw loadError ( "Not a PRG inside X00" );

	if ( bufLen < sizeof ( X00Header ) + 2 )
		throw loadError ( ERR_TRUNCATED );

	auto	tune = new p00;
	tune->load ( format, &pHeader );

	return tune;
}
//-----------------------------------------------------------------------------

void p00::load ( const char* format, const X00Header* pHeader )
{
	info.m_formatString = format;

	{
		// Decode file name
		auto	spPet = std::string_view ( (char*)pHeader->name, std::size ( pHeader->name ) );

		info.m_infoString.push_back ( petsciiToAscii ( spPet ) );
	}

	// Automatic settings
	fileOffset = X00_ID_LEN + X00_NAME_LEN + 1;
	info.m_songs = 1;
	info.m_startSong = 1;
	info.m_compatibility = SidTuneInfo::COMPATIBILITY_BASIC;

	// Create the speed/clock setting table.
	convertOldStyleSpeedToTables ( ~0, info.m_clockSpeed );
}
//-----------------------------------------------------------------------------

}
