/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright (C) 2013-2016 Leandro Nini
 * Copyright (C) 2001 Dag Lem
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "reloc65.h"

#include <cstdint>
#include <cstring>
#include <cassert>

// 16 bit header
constexpr auto	HEADER_SIZE = 8 + 9 * 2;

// Magic number
const uint8_t o65hdr[] = { 1, 0, 'o', '6', '5' };

/**
 * Read a 16 bit word from a buffer at specific location.
 *
 * @param buffer
 * @param idx
 */
inline int getWord ( uint8_t* buffer, int idx )
{
	return buffer[ idx ] | ( buffer[ idx + 1 ] << 8 );
}

/**
 * Write a 16 bit word into a buffer at specific location.
 *
 * @param buffer
 * @param idx
 * @param value
 */
inline void setWord ( uint8_t* buffer, int idx, int value )
{
	buffer[ idx ] = value & 0xff;
	buffer[ idx + 1 ] = ( value >> 8 ) & 0xff;
}

/**
 * Get the size of header options section.
 *
 * @param buf
 */
inline int read_options ( uint8_t* buf )
{
	auto	l = 0;

	auto	c = buf[ 0 ];
	while ( c )
	{
		l += c;
		c = buf[ l ];
	}
	return ++l;
}

/**
 * Get the size of undefined references list.
 *
 * @param buf
 */
inline int read_undef ( uint8_t* buf )
{
	auto	l = 2;

	auto	n = getWord ( buf, 0 );
	while ( n )
	{
		n--;
		while ( ! buf[ l++ ] );
	}
	return l;
}
//-----------------------------------------------------------------------------

reloc65::reloc65 ( int addr )
	: m_tbase ( addr )
{
}
//-----------------------------------------------------------------------------

bool reloc65::reloc ( uint8_t** buf, int* fsize )
{
	auto	tmpBuf = *buf;

	if ( std::memcmp ( tmpBuf, o65hdr, 5 ) )
		return false;

	const auto	mode = getWord ( tmpBuf, 6 );
	if (	( mode & 0x2000 )		// 32 bit size not supported
		 || ( mode & 0x4000 ) )		// pagewise relocation not supported
	{
		return false;
	}

	const auto	hlen = HEADER_SIZE + read_options ( tmpBuf + HEADER_SIZE );

	const auto	tbase = getWord ( tmpBuf, 8 );
	const auto	tlen = getWord ( tmpBuf, 10 );
	m_tdiff = m_tbase - tbase;

	const auto	dlen = getWord ( tmpBuf, 14 );

	auto	segt = tmpBuf + hlen;                    // Text segment
	auto	segd = segt + tlen;                      // Data segment
	auto	utab = segd + dlen;                      // Undefined references list

	auto	rttab = utab + read_undef ( utab );         // Text relocation table
	auto	rdtab = reloc_seg ( segt, tlen, rttab );    // Data relocation table
	auto	extab = reloc_seg ( segd, dlen, rdtab );    // Exported globals list

	reloc_globals ( extab );

	setWord ( tmpBuf, 8, m_tbase );

	*buf = segt;
	*fsize = tlen;

	return true;
}
//-----------------------------------------------------------------------------

int reloc65::reldiff ( uint8_t s )
{
	return s == 2 ? m_tdiff : 0;
}
//-----------------------------------------------------------------------------

unsigned char* reloc65::reloc_seg ( uint8_t* buf, int len, uint8_t* rtab )
{
	auto	adr = -1;
	while ( *rtab )
	{
		if ( ( *rtab & 255 ) == 255 )
		{
			adr += 254;
			rtab++;
		}
		else
		{
			adr += *rtab & 255;
			rtab++;
			const auto	type = uint8_t ( *rtab & 0xe0 );
			const auto	seg = uint8_t ( *rtab & 0x07 );
			rtab++;
			switch ( type )
			{
				case 0x80:
				{
					const int oldVal = getWord ( buf, adr );
					const int newVal = oldVal + reldiff ( seg );
					setWord ( buf, adr, newVal );
					break;
				}

				case 0x40:
				{
					const int oldVal = buf[ adr ] * 256 + *rtab;
					const int newVal = oldVal + reldiff ( seg );
					buf[ adr ] = ( newVal >> 8 ) & 255;
					*rtab = newVal & 255;
					rtab++;
					break;
				}

				case 0x20:
				{
					const int oldVal = buf[ adr ];
					const int newVal = oldVal + reldiff ( seg );
					buf[ adr ] = newVal & 255;
					break;
				}
			}

			if ( ! seg )
				rtab += 2;
		}

		if ( adr > len )
		{
			// Warning: relocation table entries past segment end!
			assert ( false );
		}
	}

	return ++rtab;
}
//-----------------------------------------------------------------------------

uint8_t* reloc65::reloc_globals ( uint8_t* buf )
{
	auto	n = getWord ( buf, 0 );
	buf += 2;

	while ( n )
	{
		while ( *( buf++ ) );
		auto	seg = *buf;
		const auto	oldVal = getWord ( buf, 1 );
		const auto	newVal = oldVal + reldiff ( seg );
		setWord ( buf, 1, newVal );
		buf += 3;
		n--;
	}

	return buf;
}
//-----------------------------------------------------------------------------
