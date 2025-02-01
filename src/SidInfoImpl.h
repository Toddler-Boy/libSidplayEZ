#pragma once
/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2011-2015 Leandro Nini
 *  Copyright 2007-2010 Antti Lankila
 *  Copyright 2000 Simon White
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

#include <stdint.h>
#include <vector>
#include <string>

#include "sidplayfp/SidInfo.h"
#include "mixer.h"

 /**
  * The implementation of the SidInfo interface.
  */
class SidInfoImpl final : public SidInfo
{
public:
	const std::string	m_name = "libsidplayEZ";
	const std::string	m_version = "2.12.0";

	std::vector<std::string>	m_credits = {
		m_name + " " + m_version + " Engine:\n",
		"Copyright (C) 2000 Simon White\n",
		"Copyright (C) 2007-2010 Antti Lankila\n",
		"Copyright (C) 2010-2024 Leandro Nini\n",
		"Copyright (C) 2023-2025 Michael Hartmann\n",
		"https://github.com/reFX/libsidplayEZ/\n",
	};

	std::string m_speedString;

	std::string m_kernalDesc;
	std::string m_basicDesc;
	std::string m_chargenDesc;

	const unsigned int	m_maxsids = libsidplayfp::Mixer::MAX_SIDS;

	uint16_t m_driverAddr = 0;
	uint16_t m_driverLength = 0;

	uint16_t m_powerOnDelay = 0;

private:
	// prevent copying
	SidInfoImpl ( const SidInfoImpl& ) = delete;
	SidInfoImpl& operator=( SidInfoImpl& ) = delete;

public:
	SidInfoImpl () = default;

	[[ nodiscard ]] const char* getName () const override { return m_name.c_str (); }
	[[ nodiscard ]] const char* getVersion () const override { return m_version.c_str (); }

	[[ nodiscard ]] unsigned int getNumberOfCredits () const override { return (unsigned int)m_credits.size (); }
	[[ nodiscard ]] const char* getCredits ( unsigned int i ) const override { return i < (unsigned int)m_credits.size () ? m_credits[ i ].c_str () : ""; }

	[[ nodiscard ]] unsigned int getMaxsids () const override { return m_maxsids; }

	[[ nodiscard ]] uint16_t getDriverAddr () const override { return m_driverAddr; }
	[[ nodiscard ]] uint16_t getDriverLength () const override { return m_driverLength; }

	[[ nodiscard ]] uint16_t getPowerOnDelay () const override { return m_powerOnDelay; }

	[[ nodiscard ]] const char* getSpeedString () const override { return m_speedString.c_str (); }

	[[ nodiscard ]] const char* getKernalDesc () const override { return m_kernalDesc.c_str (); }
	[[ nodiscard ]] const char* getBasicDesc () const override { return m_basicDesc.c_str (); }
	[[ nodiscard ]] const char* getChargenDesc () const override { return m_chargenDesc.c_str (); }
};
//-----------------------------------------------------------------------------
