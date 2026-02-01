#pragma once
/*
 * This file is part of libsidplayEZ, a SID player engine based on libsidplayfp.
 *
 * Copyright 2024-2026 Michael Hartmann <mike@ultrasid.com>
 * Copyright 2011-2026 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

/**
 * SidConfig
 *
 * An instance of this class is used to transport emulator settings
 * to and from the interface class.
 */
struct SidConfig final
{
	// SID chip model
	using sid_model_t = enum : std::uint8_t
	{
		MOS6581,       ///< Old SID (MOS 6581)
		MOS8580        ///< New SID (CSG 8580/MOS 6582)
	};

	// CIA chip model
	using cia_model_t = enum : std::uint8_t
	{
		MOS6526,       ///< Old CIA with interrupts delayed by one cycle (MOS 6526/6526A)
		MOS8521,       ///< New CIA (CSG 8521/MOS 6526 216A)
		MOS6526W4485   ///< Old CIA, peculiar batch with different serial port behavior (MOS 6526 4485) @since 2.2
	};

	// C64 model
	using c64_model_t = enum : std::uint8_t
	{
		PAL,           ///< European PAL model (MOS 6569)
		NTSC,          ///< American/Japanese NTSC model (MOS 6567 R8)
		OLD_NTSC,      ///< Older NTSC model with different video chip revision (MOS 6567 R56A)
		DREAN,         ///< Argentinian PAL-N model (MOS 6572)
		PAL_M          ///< Brazilian PAL-M model (MOS 6573)
	};

	static constexpr	uint16_t MAX_POWER_ON_DELAY = 0x1FFF;
	static constexpr	uint32_t DEFAULT_SAMPLING_FREQ = 44100;

	c64_model_t	defaultC64Model = PAL;
	bool		forceC64Model = false;
	sid_model_t	defaultSidModel = MOS6581;
	bool		forceSidModel = false;
	cia_model_t	ciaModel = MOS6526;
	uint32_t	frequency = DEFAULT_SAMPLING_FREQ;
	uint16_t	secondSidAddress = 0;
	uint16_t	thirdSidAddress = 0;
	bool		useFilter = true;

	[[ nodiscard ]] bool compare ( const SidConfig& config ) const
	{
		return		defaultC64Model		!= config.defaultC64Model
				||	forceC64Model		!= config.forceC64Model
				||	defaultSidModel		!= config.defaultSidModel
				||	forceSidModel		!= config.forceSidModel
				||	ciaModel			!= config.ciaModel
				||	frequency			!= config.frequency
				||	secondSidAddress	!= config.secondSidAddress
				||	thirdSidAddress		!= config.thirdSidAddress
				||	useFilter			!= config.useFilter;
	}
};
