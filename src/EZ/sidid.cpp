/*
* This file is part of libsidplayEZ, a SID player engine.
*
* Copyright 2025-2026 Michael Hartmann
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

#include "sidid.h"
#include "../stringutils.h"

#include <cstring>

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

bool sidid::loadSidIDConfig ( const char* filename )
{
	return loadSidIDConfigText ( stringutils::loadFile ( filename ) );
}
//-----------------------------------------------------------------------------

bool sidid::loadSidIDConfigText ( const std::string& str )
{
	if ( str.empty () )
		return false;

	// Clear old data
	sidIDs.clear ();

	// Break file into individual lines
	const auto	lines = stringutils::arrayFromTokens ( str );

	SIDID	sidID;

	auto storeSig = [ &sidID, this ]
	{
		if ( sidID.name.empty () || sidID.sigs.empty () )
			return;

		sidIDs.emplace_back ( std::move ( sidID ) );

		sidID = SIDID {};
	};

	for ( const auto& line : lines )
	{
		if ( line.find ( ' ' ) == std::string::npos )
		{
			storeSig ();
			sidID.name = line;
		}
		else
		{
			// A hand-edited config can produce an empty signature (an all-END line)
			auto	sig = parseSignature ( line );
			if ( ! sig.empty () )
				sidID.sigs.emplace_back ( std::move ( sig ) );
		}
	}
	storeSig ();

	sidIDs.shrink_to_fit ();

	return ! sidIDs.empty ();
}
//-----------------------------------------------------------------------------

sidid::signature sidid::parseSignature ( const std::string& line )
{
	signature	sig;
	fragment	frag;

	auto storeFrag = [ &sig, &frag ]
	{
		if ( frag.bytes.empty () )
			return;

		for ( size_t j = 0; j < frag.bytes.size (); j++ )
			if ( frag.bytes[ j ] != token::ANY )
			{
				frag.anchorPos = int ( j );
				frag.anchorByte = uint8_t ( frag.bytes[ j ] );
				break;
			}

		sig.emplace_back ( std::move ( frag ) );

		frag = fragment {};
	};

	for ( const auto& part : stringutils::arrayFromTokens ( line, ' ' ) )
	{
		if ( part.size () == 3 && stringutils::equal ( part, "AND" ) )
			storeFrag ();
		else if ( part.size () == 3 && stringutils::equal ( part, "END" ) )
			continue;
		else if ( part == "??" )
			frag.bytes.emplace_back ( token::ANY );
		else
			frag.bytes.emplace_back ( int16_t ( std::strtol ( part.data (), nullptr, 16 ) ) );
	}
	storeFrag ();

	return sig;
}
//-----------------------------------------------------------------------------

// Find the first occurrence of a fragment at/after pos; on success pos moves
// to the match start. memchr hunts for the fragment's anchor byte (SIMD in
// the CRT), then the candidate is verified wildcard-aware
static bool findFragment ( const uint8_t* data, size_t length, size_t& pos, const sidid::fragment& frag )
{
	const auto	fragSize = frag.bytes.size ();
	if ( pos + fragSize > length )
		return false;

	// An all-wildcard fragment matches anywhere it fits
	if ( frag.anchorPos < 0 )
		return true;

	const auto	limit = length - fragSize;

	for ( auto c = pos; c <= limit; c++ )
	{
		const auto	hit = static_cast<const uint8_t*> ( std::memchr ( data + c + frag.anchorPos, frag.anchorByte, limit - c + 1 ) );
		if ( ! hit )
			return false;

		c = size_t ( hit - data ) - frag.anchorPos;

		auto	match = true;
		for ( size_t j = 0; j < fragSize; j++ )
		{
			if ( frag.bytes[ j ] != sidid::token::ANY && data[ c + j ] != frag.bytes[ j ] )
			{
				match = false;
				break;
			}
		}

		if ( match )
		{
			pos = c;
			return true;
		}
	}
	return false;
}
//-----------------------------------------------------------------------------

std::optional<size_t> sidid::findSignature ( const uint8_t* data, size_t length, const signature& sig )
{
	// AND-separated fragments must appear in order, each after the previous one
	size_t					pos = 0;
	std::optional<size_t>	first;

	for ( const auto& frag : sig )
	{
		if ( ! findFragment ( data, length, pos, frag ) )
			return std::nullopt;

		if ( ! first )
			first = pos;

		pos += frag.bytes.size ();
	}
	return first;
}
//-----------------------------------------------------------------------------

std::vector<std::string> sidid::findPlayerRoutines ( const std::vector<uint8_t>& tuneData ) const
{
	// No signatures loaded
	if ( sidIDs.empty () )
		return {};

	// No tune loaded
	if ( tuneData.empty () )
		return {};

	// Identify playroutine
	std::vector<std::string>	routines;

	for ( const auto& id : sidIDs )
		for ( const auto& sig : id.sigs )
			if ( findSignature ( tuneData.data (), tuneData.size (), sig ) )
				if ( std::ranges::find ( routines, id.name ) == routines.end () )
					routines.emplace_back ( id.name );

	return routines;
}
//-----------------------------------------------------------------------------

}
