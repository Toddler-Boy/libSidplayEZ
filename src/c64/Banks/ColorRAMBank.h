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

#include <stdint.h>
#include <cstring>
#include <algorithm>

#include "Bank.h"

#include "../../EZ/config.h"

namespace libsidplayfp
{

/**
 * Color RAM.
 *
 * 1K x 4-bit Static RAM that stores text screen color information.
 *
 * Located at $D800-$DBFF (last 24 bytes are unused)
 */
class ColorRAMBank final : public Bank
{
private:
	uint8_t	ram[ 0x0400 ];

public:
	void reset () { std::fill ( std::begin ( ram ), std::end ( ram ), 0 ); }

	sidinline void poke ( uint16_t address, uint8_t value ) override { ram[ address & 0x3ff ] = value & 0xf; }
	sidinline uint8_t peek ( uint16_t address ) override { return ram[ address & 0x3ff ]; }
};

}
