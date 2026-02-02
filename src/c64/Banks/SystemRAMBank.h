#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2012-2021 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2010 Antti Lankila
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
#include <algorithm>

#include "Bank.h"

#include "../../EZ/config.h"

namespace libsidplayfp
{

/**
* Area backed by RAM.
*/
class SystemRAMBank final : public Bank
{
	friend class MMU;

private:
	/// C64 RAM area
	uint8_t ram[ 0x10000 ];

public:
	/**
	* Initialize RAM with powerup pattern.
	*
	* $0000: 00 00 ff ff ff ff 00 00 00 00 ff ff ff ff 00 00
	* ...
	* $4000: ff ff 00 00 00 00 ff ff ff ff 00 00 00 00 ff ff
	* ...
	* $8000: 00 00 ff ff ff ff 00 00 00 00 ff ff ff ff 00 00
	* ...
	* $c000: ff ff 00 00 00 00 ff ff ff ff 00 00 00 00 ff ff
	*/
	void reset () noexcept
	{
		uint8_t byte = 0x00;
		for ( auto j = 0x0000; j < 0x10000; j += 0x4000 )
		{
			std::fill_n ( ram + j, 0x4000, byte );

			byte = ~byte;

			for ( auto i = 0x02; i < 0x4000; i += 0x08 )
				std::fill_n ( ram + j + i, 0x04, byte );
		}
	}

	sidinline uint8_t peek ( uint16_t address ) noexcept override				{	return ram[ address ];	}
	sidinline void poke ( uint16_t address, uint8_t value ) noexcept override	{	ram[ address ] = value;	}
};

}
