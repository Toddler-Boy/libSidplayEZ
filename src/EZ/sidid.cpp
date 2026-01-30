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

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

bool sidid::loadSidIDConfig ( const char* filename )
{
	auto	str = stringutils::loadFile ( filename );
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
			sidID.sigs.push_back ( {} );
			auto& sig = sidID.sigs.back ();

			for ( const auto& part : parts )
			{
				if ( part.size () == 3 && stringutils::equal ( part, "AND" ) )
					sig.emplace_back ( SIDID::token::AND );
				else if ( part.size () == 3 && stringutils::equal ( part, "END" ) )
					sig.shrink_to_fit ();
				else if ( part == "??" )
					sig.emplace_back ( SIDID::token::ANY );
				else
					sig.emplace_back ( int16_t ( std::strtol ( part.data (), nullptr, 16 ) ) );
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

	auto identifybytes = [ &tuneData ] ( const std::vector<int16_t>& bytes ) -> bool
	{
		const auto	buffer = static_cast<const uint8_t* const> ( tuneData.data () );
		const auto	length = int ( tuneData.size () );
		const auto	sigSize = int ( bytes.size () );

		int	c = 0;
		int	d = 0;
		int	rc = 0;
		int	rd = 0;

		while ( c < length )
		{
			if ( d == rd )
			{
				if ( buffer[ c ] == bytes[ d ] )
				{
					rc = c + 1;
					d++;
				}
				c++;
			}
			else
			{
				if ( d == sigSize )
					return true;

				if ( bytes[ d ] == SIDID::token::AND )
				{
					d++;
					while ( c < length )
					{
						if ( buffer[ c ] == bytes[ d ] )
						{
							rc = c + 1;
							rd = d;
							break;
						}
						c++;
					}
					if ( c >= length )
						return false;
				}
				if ( bytes[ d ] != SIDID::token::ANY && buffer[ c ] != bytes[ d ] )
				{
					c = rc;
					d = rd;
				}
				else
				{
					c++;
					d++;
				}
			}
		}

		return d == sigSize;
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