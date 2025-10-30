#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2004,2010 Dag Lem
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

#include "FilterModelConfig.h"

#include <memory>

namespace reSIDfp
{

/**
* Calculate parameters for 8580 filter emulation.
*/
class FilterModelConfig8580 final : public FilterModelConfig
{
private:
	/**
	 * Reference voltage generated from Vcc by a voltage divider
	 */
	static constexpr auto	Vref = 4.75;

	/**
	 * Power bricks generate voltages slightly out of spec
	 */
	static constexpr auto	VOLTAGE_SKEW = 1.01;

public:
	FilterModelConfig8580 ();

public:
	[[ nodiscard ]] sidinline constexpr double getVref () const { return Vref * VOLTAGE_SKEW; }

	[[ nodiscard ]] sidinline int getNormalizedVoice ( float value ) const
	{
		const auto	tmp = N16 * ( ( value * voice_voltage_range + getVref () ) - vmin );

		assert ( tmp >= 0.0 && tmp < 65536.0 );
		return int ( tmp );
	}
};

} // namespace reSIDfp
