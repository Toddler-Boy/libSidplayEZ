#pragma once
/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <stdint.h>

/**
 * SidConfig
 *
 * An instance of this class is used to transport emulator settings
 * to and from the interface class.
 */
struct SidConfig final
{
	// SID chip model
	typedef enum
	{
		MOS6581,       ///< Old SID (MOS 6581)
		MOS8580        ///< New SID (CSG 8580/MOS 6582)
	} sid_model_t;

	// CIA chip model
	typedef enum
	{
		MOS6526,       ///< Old CIA with interrupts delayed by one cycle (MOS 6526/6526A)
		MOS8521,       ///< New CIA (CSG 8521/MOS 6526 216A)
		MOS6526W4485   ///< Old CIA, peculiar batch with different serial port behavior (MOS 6526 4485) @since 2.2
	} cia_model_t;

	// C64 model
	typedef enum
	{
		PAL,           ///< European PAL model (MOS 6569)
		NTSC,          ///< American/Japanese NTSC model (MOS 6567 R8)
		OLD_NTSC,      ///< Older NTSC model with different video chip revision (MOS 6567 R56A)
		DREAN,         ///< Argentinian PAL-N model (MOS 6572)
		PAL_M          ///< Brazilian PAL-M model (MOS 6573)
	} c64_model_t;

	/**
	 * Maximum power on delay.
	 * - Delays <= MAX produce constant results
	 * - Delays >  MAX produce random results
	 */
	static constexpr	uint16_t MAX_POWER_ON_DELAY = 0x1FFF;
	static constexpr	uint32_t DEFAULT_SAMPLING_FREQ = 44100;

	/**
	 * Intended c64 model when unknown or forced.
	 */
	c64_model_t defaultC64Model = PAL;

	/**
	 * Force the model to #defaultC64Model ignoring tune's clock setting.
	 */
	bool forceC64Model = false;

	/**
	 * Intended sid model when unknown or forced.
	 */
	sid_model_t defaultSidModel = MOS6581;

	/**
	 * Force the sid model to #defaultSidModel.
	 */
	bool forceSidModel = false;

	/**
	 * Intended cia model.
	 */
	cia_model_t ciaModel = MOS6526;

	/**
	 * Sampling frequency.
	 */
	uint32_t frequency = DEFAULT_SAMPLING_FREQ;

	/**
	 * Extra SID chips addresses.
	 */
	//@{
	uint16_t secondSidAddress = 0;
	uint16_t thirdSidAddress = 0;
	//@}

	bool useFilter = true;

	/**
	 * Compare two config objects.
	 *
	 * @return true if different
	 */
	[[ nodiscard ]] bool compare ( const SidConfig& config ) const
	{
		return		defaultC64Model != config.defaultC64Model
				||	forceC64Model != config.forceC64Model
				||	defaultSidModel != config.defaultSidModel
				||	forceSidModel != config.forceSidModel
				||	ciaModel != config.ciaModel
				||	frequency != config.frequency
				||	secondSidAddress != config.secondSidAddress
				||	thirdSidAddress != config.thirdSidAddress
				||	useFilter != config.useFilter;
	}
};
