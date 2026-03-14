#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
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

#include "EZ/config.h"

namespace
{
	// Set the lo byte (8 bit) in a word (16 bit)
	sidinline void set_16lo8 ( uint16_t& word, uint8_t byte )		{	word = ( word & 0xff00 ) | byte;	}

	// Get the lo byte (8 bit) in a word (16 bit)
	[[ nodiscard ]] sidinline uint8_t get_16lo8 ( uint16_t word )	{	return uint8_t ( word );			}

	// Convert high-byte and low-byte to 16-bit word.
	[[ nodiscard ]] sidinline uint16_t get_16 ( uint8_t hi, uint8_t lo ) { return uint16_t ( ( hi << 8 ) | lo ); }

	// Set the low-byte (8 bit) and the high-byte to 16-bit little endian word
	sidinline void set_little16 ( uint8_t ptr[ 2 ], uint16_t word )
	{
		ptr[ 0 ] = uint8_t ( word );
		ptr[ 1 ] = uint8_t ( word >> 8 );
	}

	// Set the hi byte (8 bit) in a word (16 bit)
	sidinline void set_16hi8 ( uint16_t& word, uint8_t byte )		{	word = ( word & 0x00FF ) | uint16_t ( byte << 8 );	}

	// Get the hi byte (8 bit) in a word (16 bit)
	[[ nodiscard ]] sidinline uint8_t get_16hi8 ( uint16_t word )	{	return uint8_t ( word >> 8 );		}

	// Convert high-byte and low-byte to 16-bit little endian word
	[[ nodiscard ]] sidinline uint16_t get_little16 ( uint8_t ptr[ 2 ] )	{	return uint16_t ( ( ptr[ 1 ] << 8 ) | ptr[ 0 ] );	}
}
