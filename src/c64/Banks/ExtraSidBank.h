#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2012-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "Bank.h"
#include <vector>
#include <algorithm>

#include "../../sidemu.h"

#include "../../EZ/config.h"

namespace libsidplayfp
{

/**
* Extra SID bank.
*/
class ExtraSidBank final : public Bank
{
private:
	// Size of mapping table. Each 32 bytes another SID chip base address can be assigned to
	static constexpr int	MAPPER_SIZE = 256 / 32;

	/**
	* SID mapping table.
	* Maps a SID chip base address to a SID
	* or to the underlying bank.
	*/
	Bank*	mapper[ MAPPER_SIZE ] = {};

	std::vector<sidemu*>	sids;

public:
	void reset () noexcept
	{
		for ( auto sid : sids )
			sid->reset ( 0xF );
	}

	void resetSIDMapper ( Bank* bank ) noexcept {	std::fill_n ( mapper, MAPPER_SIZE, bank );	}

	sidinline uint8_t peek ( uint16_t addr ) noexcept override				{	return mapper[ addr >> 5 & ( MAPPER_SIZE - 1 ) ]->peek ( addr );	}
	sidinline void poke ( uint16_t addr, uint8_t data ) noexcept override	{	mapper[ addr >> 5 & ( MAPPER_SIZE - 1 ) ]->poke ( addr, data );		}

	/**
	* Set SID emulation.
	*
	* @param s the emulation
	* @param address the address where to put the chip
	*/
	void addSID ( sidemu* s, uint16_t address ) noexcept
	{
		sids.push_back ( s );
		mapper[ address >> 5 & ( MAPPER_SIZE - 1 ) ] = s;
	}
};

}
