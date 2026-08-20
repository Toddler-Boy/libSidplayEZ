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

		sidID.sigs.shrink_to_fit ();
		sidIDs.emplace_back ( sidID );

		sidID = SIDID {};
	};

	for ( const auto& line : lines )
	{
		const auto	parts = stringutils::arrayFromTokens ( line, ' ' );
		if ( parts.size () == 1 )
		{
			storeSig ();
			sidID.name = parts[ 0 ];
		}
		else
		{
			SIDID::signature	sig;
			SIDID::fragment		frag;

			auto storeFrag = [ &sig, &frag ]
			{
				if ( frag.bytes.empty () )
					return;

				for ( size_t j = 0; j < frag.bytes.size (); j++ )
					if ( frag.bytes[ j ] != SIDID::token::ANY )
					{
						frag.anchorPos = int ( j );
						frag.anchorByte = uint8_t ( frag.bytes[ j ] );
						break;
					}

				frag.bytes.shrink_to_fit ();
				sig.emplace_back ( std::move ( frag ) );

				frag = SIDID::fragment {};
			};

			for ( const auto& part : parts )
			{
				if ( part.size () == 3 && stringutils::equal ( part, "AND" ) )
					storeFrag ();
				else if ( part.size () == 3 && stringutils::equal ( part, "END" ) )
					continue;
				else if ( part == "??" )
					frag.bytes.emplace_back ( SIDID::token::ANY );
				else
					frag.bytes.emplace_back ( int16_t ( std::strtol ( part.data (), nullptr, 16 ) ) );
			}
			storeFrag ();

			// A hand-edited config can produce an empty signature (an all-END line)
			if ( ! sig.empty () )
			{
				sig.shrink_to_fit ();
				sidID.sigs.emplace_back ( std::move ( sig ) );
			}
		}
	}
	storeSig ();

	sidIDs.shrink_to_fit ();

	return ! sidIDs.empty ();
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

	const auto	buffer = static_cast<const uint8_t* const> ( tuneData.data () );
	const auto	length = tuneData.size ();

	// Find the first occurrence of a fragment at/after pos; on success pos
	// advances to just past the match. memchr hunts for the fragment's anchor
	// byte (SIMD in the CRT), then the candidate is verified wildcard-aware
	auto findFragment = [ buffer, length ] ( size_t& pos, const SIDID::fragment& frag ) -> bool
	{
		const auto	fragSize = frag.bytes.size ();
		if ( pos + fragSize > length )
			return false;

		// An all-wildcard fragment matches anywhere it fits
		if ( frag.anchorPos < 0 )
		{
			pos += fragSize;
			return true;
		}

		const auto	limit = length - fragSize;

		for ( auto c = pos; c <= limit; c++ )
		{
			const auto	hit = static_cast<const uint8_t*> ( std::memchr ( buffer + c + frag.anchorPos, frag.anchorByte, limit - c + 1 ) );
			if ( ! hit )
				return false;

			c = size_t ( hit - buffer ) - frag.anchorPos;

			auto	match = true;
			for ( size_t j = 0; j < fragSize; j++ )
				if ( frag.bytes[ j ] != SIDID::token::ANY && buffer[ c + j ] != frag.bytes[ j ] )
				{
					match = false;
					break;
				}

			if ( match )
			{
				pos = c + fragSize;
				return true;
			}
		}
		return false;
	};

	// AND-separated fragments must appear in order, each after the previous one
	auto identifybytes = [ &findFragment ] ( const SIDID::signature& sig ) -> bool
	{
		size_t	pos = 0;

		for ( const auto& frag : sig )
			if ( ! findFragment ( pos, frag ) )
				return false;

		return true;
	};

	// Identify playroutine
	std::vector<std::string>	routines;

	for ( const auto& id : sidIDs )
		for ( const auto& sig : id.sigs )
			if ( identifybytes ( sig ) )
				if ( std::ranges::find ( routines, id.name ) == routines.end () )
					routines.emplace_back ( id.name );

	return routines;
}
//-----------------------------------------------------------------------------

}