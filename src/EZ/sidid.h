#pragma once
/*
* This file is part of libsidplayEZ, a SID player engine.
*
* Copyright 2025 Michael Hartmann
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
#include <string>
#include <vector>

namespace libsidplayEZ
{

class sidid
{
public:
	bool loadSidIDConfig ( const char* filename );
	std::vector<std::string> findPlayerRoutines ( const std::vector<uint8_t>& data ) const;

private:
	struct SIDID
	{
		enum token : int16_t
		{
			ANY = -1,
			AND = -2,
			END = -3,
		};

		std::string							name;
		std::vector<std::vector<int16_t>>	sigs;
	};

	std::vector<SIDID>	sidIDs;
};
//-----------------------------------------------------------------------------

}
