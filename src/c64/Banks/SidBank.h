#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "Bank.h"
#include "../../sidemu.h"

#include "../../EZ/config.h"

namespace libsidplayfp
{

/**
* SID
*
* Located at $D400-$D7FF, mirrored each 32 bytes
*/
class SidBank final : public Bank
{
public:
	void reset () noexcept {	sid->reset ( 0xf );	}

	sidinline uint8_t peek ( uint16_t addr ) noexcept override { return sid->peek ( addr ); }
	sidinline void poke ( uint16_t addr, uint8_t data ) noexcept override { sid->poke ( addr, data ); }

	/**
	* Set SID emulation.
	*
	* @param s the emulation, nullptr to remove current sid
	*/
	void setSID ( sidemu* s ) noexcept {	sid = s;	}

private:
	// SID chip
	sidemu*	sid = nullptr;
};

}
