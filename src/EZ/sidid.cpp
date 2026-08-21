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

#include <algorithm>
#include <cstring>
#include <fstream>

namespace sidid
{

//-----------------------------------------------------------------------------
// Local file/line helpers keep sidid.h/.cpp a self-contained two-file drop-in

static std::string loadFile ( const char* filename )
{
	auto	file = std::ifstream ( filename, std::ios::in | std::ios::binary | std::ios::ate );
	if ( ! file.is_open () )
		return {};

	const auto	size = file.tellg ();
	if ( size <= 0 )
		return {};

	file.seekg ( 0, std::ios::beg );

	auto	str = std::string ( size_t ( size ), '\0' );
	file.read ( str.data (), size );

	return str;
}
//-----------------------------------------------------------------------------

static std::vector<std::string> tokenize ( const std::string& input, const char delimiter )
{
	std::vector<std::string>	tokens;

	size_t	pos = 0;
	while ( pos <= input.size () )
	{
		auto	end = input.find ( delimiter, pos );
		if ( end == std::string::npos )
			end = input.size ();

		auto	first = pos;
		auto	last = end;
		while ( first < last && ( input[ first ] == ' ' || input[ first ] == '\t' || input[ first ] == '\r' ) )
			first++;
		while ( last > first && ( input[ last - 1 ] == ' ' || input[ last - 1 ] == '\t' || input[ last - 1 ] == '\r' ) )
			last--;

		if ( last > first )
			tokens.emplace_back ( input.substr ( first, last - first ) );

		pos = end + 1;
	}

	return tokens;
}
//-----------------------------------------------------------------------------

signature parseSignature ( const std::string& line )
{
	signature	sig;
	fragment	frag;

	auto storeFrag = [ &sig, &frag ]
	{
		if ( frag.bytes.empty () )
			return;

		for ( size_t j = 0; j < frag.bytes.size (); j++ )
		{
			if ( frag.bytes[ j ] != token::ANY )
			{
				frag.anchorPos = int ( j );
				frag.anchorByte = uint8_t ( frag.bytes[ j ] );
				break;
			}
		}

		sig.emplace_back ( std::move ( frag ) );

		frag = fragment {};
	};

	for ( const auto& part : tokenize ( line, ' ' ) )
	{
		if ( part == "AND" || part == "&&" )
			storeFrag ();
		else if ( part == "END" )
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
static bool findFragment ( const uint8_t* data, size_t length, size_t& pos, const fragment& frag )
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
			if ( frag.bytes[ j ] != token::ANY && data[ c + j ] != frag.bytes[ j ] )
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

std::optional<size_t> findSignature ( const uint8_t* data, size_t length, const signature& sig )
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

bool database::loadSidIDConfig ( const char* filename )
{
	return loadSidIDConfigText ( loadFile ( filename ) );
}
//-----------------------------------------------------------------------------

bool database::loadSidIDConfigText ( const std::string& str )
{
	if ( str.empty () )
		return false;

	// Clear old data
	players.clear ();

	// Break file into individual lines
	const auto	lines = tokenize ( str, '\n' );

	player	entry;

	auto storeEntry = [ &entry, this ]
	{
		if ( entry.name.empty () || entry.sigs.empty () )
			return;

		players.emplace_back ( std::move ( entry ) );

		entry = player {};
	};

	for ( const auto& line : lines )
	{
		if ( line.find ( ' ' ) == std::string::npos )
		{
			storeEntry ();
			entry.name = line;
		}
		else
		{
			// A hand-edited config can produce an empty signature (an all-END line)
			auto	sig = parseSignature ( line );
			if ( ! sig.empty () )
				entry.sigs.emplace_back ( std::move ( sig ) );
		}
	}
	storeEntry ();

	players.shrink_to_fit ();

	buildGate ();

	return ! players.empty ();
}
//-----------------------------------------------------------------------------

// The .nfo is Latin-1; the conversion is lossless
static std::string latin1ToUTF8 ( const std::string& in )
{
	std::string	out;
	out.reserve ( in.size () );

	for ( const auto sc : in )
	{
		const auto	c = uint8_t ( sc );
		if ( c < 128 )
		{
			out += char ( c );
		}
		else
		{
			out += char ( 0xC0 | ( c >> 6 ) );
			out += char ( 0x80 | ( c & 0x3F ) );
		}
	}
	return out;
}
//-----------------------------------------------------------------------------

bool database::loadSidIDInfo ( const char* filename )
{
	return loadSidIDInfoText ( loadFile ( filename ) );
}
//-----------------------------------------------------------------------------

bool database::loadSidIDInfoText ( const std::string& str )
{
	playerInfos.clear ();

	if ( str.empty () )
		return false;

	playerInfo		info;
	std::string*	lastField = nullptr;

	auto storeInfo = [ &info, this ]
	{
		if ( ! info.player.empty () )
			playerInfos.emplace_back ( std::move ( info ) );

		info = playerInfo {};
	};

	size_t	pos = 0;
	while ( pos < str.size () )
	{
		auto	end = str.find ( '\n', pos );
		if ( end == std::string::npos )
			end = str.size ();

		auto	line = str.substr ( pos, end - pos );
		pos = end + 1;

		while ( ! line.empty () && ( line.back () == '\r' || line.back () == ' ' ) )
			line.pop_back ();

		if ( line.empty () )
			continue;

		// Field keys are right-aligned with the colon at column 9; anything
		// else with a colon is quote text inside a comment
		std::string*	field = nullptr;
		if ( line.size () > 10 && line[ 9 ] == ':' )
		{
			const auto	key = line.substr ( 0, 10 );
			if ( key == "     NAME:" )			field = &info.name;
			else if ( key == "   AUTHOR:" )		field = &info.author;
			else if ( key == " RELEASED:" )		field = &info.released;
			else if ( key == "  COMMENT:" )		field = &info.comment;
			else if ( key == "REFERENCE:" )		field = &info.reference;
		}

		if ( field )
		{
			auto	value = line.substr ( 10 );
			value.erase ( 0, value.find_first_not_of ( ' ' ) );

			*field = latin1ToUTF8 ( value );
			lastField = field;
		}
		else if ( line.front () != ' ' && line.front () != '\t' )
		{
			// A new player id
			storeInfo ();
			info.player = line;
			lastField = nullptr;
		}
		else if ( lastField )
		{
			// An indented line continues the previous field
			auto	value = line;
			value.erase ( 0, value.find_first_not_of ( " \t" ) );

			*lastField += '\n';
			*lastField += latin1ToUTF8 ( value );
		}
	}
	storeInfo ();

	playerInfos.shrink_to_fit ();

	return ! playerInfos.empty ();
}
//-----------------------------------------------------------------------------

const playerInfo* database::findPlayerInfo ( const std::string& player ) const
{
	for ( const auto& info : playerInfos )
		if ( info.player == player )
			return &info;

	return nullptr;
}
//-----------------------------------------------------------------------------

void database::buildGate ()
{
	gateSigs.clear ();
	gateBitmap.assign ( 65536 / 64, 0 );
	gateOffsets.assign ( 65536 + 1, 0 );
	gateEntries.clear ();

	// Flatten the config and key each signature by the first adjacent literal
	// byte pair of its first fragment; 0x10000 = no pair, always a candidate
	std::vector<uint32_t>	pairOf;

	for ( uint32_t p = 0; p < uint32_t ( players.size () ); p++ )
		for ( uint32_t s = 0; s < uint32_t ( players[ p ].sigs.size () ); s++ )
		{
			const auto&	bytes = players[ p ].sigs[ s ].front ().bytes;

			auto	off = -1;
			for ( size_t j = 0; j + 1 < bytes.size (); j++ )
				if ( bytes[ j ] != token::ANY && bytes[ j + 1 ] != token::ANY )
				{
					off = int ( j );
					break;
				}

			gateSigs.push_back ( { p, s, off } );
			pairOf.push_back ( off < 0 ? 0x10000 : uint32_t ( uint8_t ( bytes[ off ] ) | uint8_t ( bytes[ off + 1 ] ) << 8 ) );
		}

	// Bucket the gated signatures by pair value; the counting sort keeps
	// config order within each bucket
	for ( const auto pair : pairOf )
	{
		if ( pair < 0x10000 )
		{
			gateBitmap[ pair >> 6 ] |= 1ull << ( pair & 63 );
			gateOffsets[ pair + 1 ]++;
		}
	}

	for ( size_t v = 1; v <= 65536; v++ )
		gateOffsets[ v ] += gateOffsets[ v - 1 ];

	gateEntries.resize ( gateOffsets[ 65536 ] );

	auto	cursor = gateOffsets;
	for ( uint32_t i = 0; i < uint32_t ( pairOf.size () ); i++ )
		if ( pairOf[ i ] < 0x10000 )
			gateEntries[ cursor[ pairOf[ i ] ]++ ] = i;
}
//-----------------------------------------------------------------------------

std::vector<database::candidate> database::findCandidates ( const uint8_t* data, size_t length ) const
{
	std::vector<candidate>	out;
	findCandidates ( data, length, out );
	return out;
}
//-----------------------------------------------------------------------------

void database::findCandidates ( const uint8_t* data, size_t length, std::vector<candidate>& out ) const
{
	out.clear ();

	if ( gateSigs.empty () || ! data )
		return;

	// The scratch is per-thread, so concurrent scans stay independent
	static thread_local std::vector<uint8_t>	seen;
	seen.assign ( gateSigs.size (), 0 );

	// One pass: mark every signature whose first fragment verifies somewhere
	for ( size_t pos = 0; pos + 1 < length; pos++ )
	{
		const auto	v = unsigned ( data[ pos ] | data[ pos + 1 ] << 8 );
		if ( ! ( gateBitmap[ v >> 6 ] & ( 1ull << ( v & 63 ) ) ) )
			continue;

		for ( auto e = gateOffsets[ v ]; e < gateOffsets[ v + 1 ]; e++ )
		{
			const auto	idx = gateEntries[ e ];
			if ( seen[ idx ] )
				continue;

			const auto&	gs = gateSigs[ idx ];
			if ( size_t ( gs.pairOffset ) > pos )
				continue;

			// Verify the whole first fragment at the candidate position
			const auto&	frag = players[ gs.player ].sigs[ gs.sig ].front ();
			const auto	start = pos - gs.pairOffset;
			if ( start + frag.bytes.size () > length )
				continue;

			auto	match = true;
			for ( size_t j = 0; j < frag.bytes.size (); j++ )
			{
				if ( frag.bytes[ j ] != token::ANY && data[ start + j ] != frag.bytes[ j ] )
				{
					match = false;
					break;
				}
			}

			if ( match )
				seen[ idx ] = 1;
		}
	}

	for ( size_t i = 0; i < gateSigs.size (); i++ )
		if ( gateSigs[ i ].pairOffset < 0 || seen[ i ] )
			out.push_back ( { gateSigs[ i ].player, &players[ gateSigs[ i ].player ].sigs[ gateSigs[ i ].sig ] } );
}
//-----------------------------------------------------------------------------

std::vector<std::string> database::findPlayerRoutines ( const std::vector<uint8_t>& tuneData ) const
{
	// No signatures loaded
	if ( players.empty () )
		return {};

	// No tune loaded
	if ( tuneData.empty () )
		return {};

	// Identify playroutine: the gate narrows the config down to the few
	// signatures whose first fragment is present at all
	std::vector<std::string>	routines;

	auto	lastPlayer = size_t ( -1 );
	for ( const auto& cand : findCandidates ( tuneData.data (), tuneData.size () ) )
	{
		if ( cand.player == lastPlayer )
			continue;

		if ( ! findSignature ( tuneData.data (), tuneData.size (), *cand.sig ) )
			continue;

		lastPlayer = cand.player;

		const auto&	name = players[ cand.player ].name;
		if ( std::ranges::find ( routines, name ) == routines.end () )
			routines.emplace_back ( name );
	}

	return routines;
}
//-----------------------------------------------------------------------------

}
