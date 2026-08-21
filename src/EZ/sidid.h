#pragma once
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

//-----------------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sidid
{

enum token : int16_t
{
	ANY = -1,
};

// A signature is a list of AND-separated fragments that must appear
// in order; each fragment is a run of bytes and ?? wildcards
struct fragment
{
	std::vector<int16_t>	bytes;			// 0..255 or token::ANY
	int						anchorPos = -1;	// first non-wildcard byte, for memchr
	uint8_t					anchorByte = 0;
};

using signature = std::vector<fragment>;

struct player
{
	std::string				name;
	std::vector<signature>	sigs;
};

// Optional player metadata parsed from sidid.nfo. Values are converted from
// the .nfo's Latin-1 to UTF-8; multi-line values keep their line breaks
struct playerInfo
{
	std::string	player;		// the sidid.cfg player id
	std::string	name;		// full player/editor name
	std::string	author;
	std::string	released;
	std::string	comment;
	std::string	reference;
};

// One signature in sidid.cfg line syntax ("20 ?? D4 AND A9 00 END"; the new
// format's "&&" is accepted for "AND", END is optional in both formats);
// empty result when the line holds no bytes
[[ nodiscard ]] signature parseSignature ( const std::string& line );

// Offset of the signature's first fragment in data, or nullopt
[[ nodiscard ]] std::optional<size_t> findSignature ( const uint8_t* data, size_t length, const signature& sig );

//-----------------------------------------------------------------------------

class database
{
public:
	bool loadSidIDConfig ( const char* filename );
	bool loadSidIDConfigText ( const std::string& str );
	[[ nodiscard ]] std::vector<std::string> findPlayerRoutines ( const std::vector<uint8_t>& data ) const;

	[[ nodiscard ]] const std::vector<player>& getPlayers () const	{ return players; }

	// sidid.nfo player metadata; loads independently of the config
	bool loadSidIDInfo ( const char* filename );
	bool loadSidIDInfoText ( const std::string& str );
	[[ nodiscard ]] const playerInfo* findPlayerInfo ( const std::string& player ) const;
	[[ nodiscard ]] const std::vector<playerInfo>& getPlayerInfos () const	{ return playerInfos; }

	// Pair-keyed prefilter: one pass over data collects the signatures whose
	// first fragment occurs, so only those need the full matcher. Conservative,
	// no false negatives; findPlayerRoutines uses it internally. Tools scan with
	//
	//   for ( const auto& c : db.findCandidates ( data, size ) )
	//       if ( const auto offset = sidid::findSignature ( data, size, *c.sig ) )
	//           ...  // db.getPlayers ()[ c.player ].name matches at *offset
	struct candidate
	{
		size_t				player;	// index into getPlayers()
		const signature*	sig;
	};

	[[ nodiscard ]] std::vector<candidate> findCandidates ( const uint8_t* data, size_t length ) const;

	// Same, reusing the caller's vector; safe to call from several threads
	void findCandidates ( const uint8_t* data, size_t length, std::vector<candidate>& out ) const;

private:
	void buildGate ();

	std::vector<player>		players;
	std::vector<playerInfo>	playerInfos;

	// The gate: every signature keyed by the first adjacent literal byte pair
	// of its first fragment (signatures without one are always candidates)
	struct gateSig
	{
		uint32_t	player;
		uint32_t	sig;
		int			pairOffset;	// pair position in the first fragment, -1 = ungated
	};

	std::vector<gateSig>	gateSigs;
	std::vector<uint64_t>	gateBitmap;		// 65536 bits: pair starts some signature
	std::vector<uint32_t>	gateEntries;	// gateSigs indices, grouped by pair value
	std::vector<uint32_t>	gateOffsets;	// pair value -> gateEntries range [v, v+1)
};
//-----------------------------------------------------------------------------

}
