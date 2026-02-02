#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2000 Simon White
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

#include "../sidmemory.h"
#include "../EventScheduler.h"

#include "Banks/pla.h"
#include "Banks/SystemRAMBank.h"
#include "Banks/SystemROMBanks.h"
#include "Banks/ZeroRAMBank.h"

#include <algorithm>

namespace libsidplayfp
{

class Bank;
class IOBank;

/**
	* The C64 MMU chip.
	*/
class MMU final : public PLA, public sidmemory
{
private:
	EventScheduler& eventScheduler;

	/// CPU port signals
	bool	loram = false;
	bool	hiram = false;
	bool	charen = false;

	friend uint8_t readIO ( MMU& self, uint16_t addr ) noexcept;
	using ReadFunc = uint8_t ( * )( MMU& self, uint16_t addr );

	/// CPU read memory mapping in 4k chunks
	ReadFunc cpuReadMap[ 16 ];

	/// CPU write memory mapping in 4k chunks
	Bank* cpuWriteMap[ 16 ];

	/// IO region handler
	IOBank* ioBank;

	/// Kernal ROM
	KernalRomBank kernalRomBank;

	/// BASIC ROM
	BasicRomBank basicRomBank;

	/// Character ROM
	CharacterRomBank characterRomBank;

	/// RAM
	SystemRAMBank ramBank;

	/// RAM bank 0
	ZeroRAMBank zeroRAMBank;

	/// random seed
	mutable unsigned int seed = 3686734;

	void setCpuPort ( uint8_t state ) noexcept override;
	uint8_t getLastReadByte () const noexcept override;
	event_clock_t getPhi2Time () const noexcept override { return eventScheduler.getTime ( EVENT_CLOCK_PHI2 ); }

	void updateMappingPHI2 () noexcept;

public:
	MMU ( EventScheduler& eventScheduler, IOBank* ioBank );

	void reset () noexcept;

	// ROM banks methods
	void setKernal ( const uint8_t* rom ) noexcept override { kernalRomBank.set ( rom ); }
	void setBasic ( const uint8_t* rom ) noexcept override { basicRomBank.set ( rom ); }
	void setChargen ( const uint8_t* rom ) noexcept override { characterRomBank.set ( rom ); }

	// RAM access methods
	uint8_t readMemByte ( uint16_t addr ) noexcept override { return ramBank.ram[addr]; }
	uint16_t readMemWord ( uint16_t addr ) noexcept override { return uint16_t ( ramBank.ram[ addr + 1 ] << 8 | ramBank.ram[ addr ] );	}

	void writeMemByte ( uint16_t addr, uint8_t value ) noexcept override { ramBank.ram[ addr ] = value; }
	void writeMemWord ( uint16_t addr, uint16_t value ) noexcept override {
		ramBank.ram[ addr ] = uint8_t ( value );
		ramBank.ram[ addr + 1 ] = uint8_t ( value >> 8 );
	}

	void fillRam ( uint16_t start, uint8_t value, unsigned int size ) noexcept override			{	std::fill_n ( ramBank.ram + start, size, value );	}
	void fillRam ( uint16_t start, const uint8_t* source, unsigned int size ) noexcept override	{	std::copy_n ( source, size, ramBank.ram + start );	}

	// SID specific hacks
	void installResetHook ( uint16_t addr ) noexcept override { kernalRomBank.installResetHook ( addr ); }

	void installBasicTrap ( uint16_t addr ) noexcept override { basicRomBank.installTrap ( addr ); }

	void setBasicSubtune ( uint8_t tune ) noexcept override { basicRomBank.setSubtune ( tune ); }

	/**
	* Access memory as seen by CPU.
	*
	* @param addr the address where to read from
	* @return value at address
	*/
	sidinline uint8_t cpuRead ( uint16_t addr ) noexcept { return ( cpuReadMap[ addr >> 12 ] )( *this, addr ); }

	/**
	* Access memory as seen by CPU.
	*
	* @param addr the address where to write
	* @param data the value to write
	*/
	sidinline void cpuWrite ( uint16_t addr, uint8_t data ) noexcept { cpuWriteMap[ addr >> 12 ]->poke ( addr, data ); }
};

}
