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

namespace libsidplayEZ
{

class sidid
{
public:
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

	struct SIDID
	{
		std::string				name;
		std::vector<signature>	sigs;
	};

	bool loadSidIDConfig ( const char* filename );
	bool loadSidIDConfigText ( const std::string& str );
	[[ nodiscard ]] std::vector<std::string> findPlayerRoutines ( const std::vector<uint8_t>& data ) const;

	// One signature in sidid.cfg line syntax ("20 ?? D4 AND A9 00 END");
	// empty result when the line holds no bytes
	[[ nodiscard ]] static signature parseSignature ( const std::string& line );

	// Offset of the signature's first fragment in data, or nullopt
	[[ nodiscard ]] static std::optional<size_t> findSignature ( const uint8_t* data, size_t length, const signature& sig );

	[[ nodiscard ]] const std::vector<SIDID>& getSidIDs () const	{ return sidIDs; }

private:
	std::vector<SIDID>	sidIDs;
};
//-----------------------------------------------------------------------------

}
