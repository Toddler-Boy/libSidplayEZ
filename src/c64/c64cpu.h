#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
*  Copyright (C) 2012-2021 Leandro Nini
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "mmu.h"

namespace libsidplayfp
{

//-----------------------------------------------------------------------------

class c64cpubus final
{
public:
	c64cpubus ( MMU& mmu )
		: m_mmu ( mmu )
	{
	}

	sidinline uint8_t cpuRead ( uint16_t addr ) noexcept				{	return m_mmu.cpuRead ( addr );	}
	sidinline void cpuWrite ( uint16_t addr, uint8_t data ) noexcept	{	m_mmu.cpuWrite ( addr, data );	}

private:
	MMU&	m_mmu;
};
//-----------------------------------------------------------------------------

}
