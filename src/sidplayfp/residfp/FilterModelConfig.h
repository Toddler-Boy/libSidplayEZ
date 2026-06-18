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

#include <cassert>
#include <memory>

#include "OpAmp.h"
#include "Spline.h"

namespace reSIDfp
{

/**
* Heap-allocated lookup tables shared across FilterModelConfig instances with
* identical configuration.  Pure data — no pointer bookkeeping here.
* FilterModelConfig keeps its own small embedded pointer arrays (mixer[8],
* summer[5]) that are filled in by assignSharedTables() to point into this
* block; that keeps the addresses returned by getMixer()/getSummer() stable
* from the moment the FilterModelConfig object is allocated.
*/
struct SharedFilterTables
{
	uint16_t	mixer0[ 1 ];
	uint16_t	mixer1[ 1 << 16 ];
	uint16_t	mixer2[ 2 << 16 ];
	uint16_t	mixer3[ 3 << 16 ];
	uint16_t	mixer4[ 4 << 16 ];
	uint16_t	mixer5[ 5 << 16 ];
	uint16_t	mixer6[ 6 << 16 ];
	uint16_t	mixer7[ 7 << 16 ];

	uint16_t	summer2[ 2 << 16 ];
	uint16_t	summer3[ 3 << 16 ];
	uint16_t	summer4[ 4 << 16 ];
	uint16_t	summer5[ 5 << 16 ];
	uint16_t	summer6[ 6 << 16 ];

	uint16_t	volume[ 16 ][ 1 << 16 ];
	uint16_t	resonance[ 16 ][ 1 << 16 ];
	uint16_t	opamp_rev[ 1 << 16 ];
};


class FilterModelConfig
{
protected:
	// Transistor parameters.
	//@{
	/// Thermal voltage: Ut = kT/q = 8.61734315e-5*T ~ 26mV
	static constexpr double		k = 1.380649e-23;    // Boltzmann Constant
	static constexpr double		q = 1.602176634e-19; // charge of an electron
	static constexpr double		temp = 27;           // temperature in °C
	static constexpr double		Ut = ( k * ( temp + 273.15 ) ) / q;

	const double Vdd;			//< Positive supply voltage
	const double Vth;			//< Threshold voltage
	const double Vddt;			//< Vdd - Vth

	double	uCox;				//< Transconductance coefficient: u*Cox
	//@}

	// Capacitor value.
	double	C;

	// Derived stuff
	const double	vmin;
	const double	vmax;
	const double	denorm;
	const double	norm;

	// Fixed point scaling for 16 bit op-amp output.
	const double N16;

	const double voice_voltage_range;

	// Current factor coefficient for op-amp integrators
	double currFactorCoeff;

	// Shared heap-allocated lookup tables.
	// Owned via shared_ptr so multiple instances with identical config reuse
	// the same allocation.  Set by derived-class constructor via assignSharedTables().
	std::shared_ptr<SharedFilterTables>	m_tables;

	// Per-instance pointer arrays whose ADDRESSES are stable from the moment
	// the FilterModelConfig object is allocated.  Their VALUES are filled in
	// by assignSharedTables() to redirect into m_tables.
	//
	// Keeping these as embedded arrays (not a raw uint16_t**) is essential:
	// Filter's base-class constructor calls getMixer()/getSummer() while the
	// derived FilterModelConfig subobject is still uninitialised (it is a
	// member that comes after the base class in the initialiser list of
	// Filter6581/Filter8580).  Returning the address of an embedded array is
	// safe even before construction; returning the value of a scalar pointer
	// member would be garbage at that point.
	uint16_t*	mixer[ 8 ]  = {};	// zero-initialised; filled by assignSharedTables()
	uint16_t*	summer[ 5 ] = {};	// zero-initialised; filled by assignSharedTables()

private:
 	double			rndBuffer[ 4096 ];
 	mutable int		rndIndex = 0;

public:
	FilterModelConfig ( const FilterModelConfig& ) = delete;
	FilterModelConfig& operator= ( const FilterModelConfig& ) = delete;

protected:
	/**
	* @param vvr voice voltage range
	* @param c   capacitor value
	* @param vdd Vdd
	* @param vth threshold voltage
	* @param ucox u*Cox
	*/
	FilterModelConfig ( double vvr, double c, double vdd, double vth, double ucox, const Spline::Point* opamp_voltage, int opamp_size );
	~FilterModelConfig () = default;

	// Transfer ownership of a SharedFilterTables block and wire up the
	// mixer/summer pointer arrays.  Must be called before any build method.
	void assignSharedTables ( std::shared_ptr<SharedFilterTables> tables ) noexcept;

	// Build the opamp_rev table into m_tables.  Extracted from the original
	// base constructor so that derived classes can call it after assignSharedTables().
	void buildOpAmpRevTable ( const Spline::Point* opamp_voltage, int opamp_size ) noexcept;

	void setUCoxAndCap ( double new_uCox, double new_C ) noexcept;

	void buildSummerTable ( OpAmp& opAmp ) noexcept;
	void buildMixerTable ( OpAmp& opampModel, double nRatio ) noexcept;
	void buildVolumeTable ( OpAmp& opampModel, double nDivisor ) noexcept;
	void buildResonanceTable ( OpAmp& opampModel, const double resonance_n[ 16 ] ) noexcept;

public:
	// Returns null if called before assignSharedTables() (e.g. from Filter's base
	// constructor while the FilterModelConfig subobject is still uninitialised).
	// Filter6581/8580 re-fetch these into their protected base members once the
	// fmc member is fully constructed.
	[[ nodiscard ]] uint16_t* getVolume () noexcept { return m_tables ? &m_tables->volume[ 0 ][ 0 ] : nullptr; }
	[[ nodiscard ]] uint16_t* getResonance () noexcept { return m_tables ? &m_tables->resonance[ 0 ][ 0 ] : nullptr; }
	[[ nodiscard ]] uint16_t** getSummer () noexcept { return summer; }
	[[ nodiscard ]] uint16_t** getMixer () noexcept { return mixer; }

	[[ nodiscard ]] sidinline uint16_t getOpampRev ( int i ) const noexcept { return m_tables->opamp_rev[ i ]; }
	[[ nodiscard ]] sidinline const uint16_t* getOpampRevPtr () const noexcept { return m_tables->opamp_rev; }
	[[ nodiscard ]] sidinline double getVddt () const noexcept { return Vddt; }
	[[ nodiscard ]] sidinline double getVth () const noexcept { return Vth; }

	// helper functions
	[[ nodiscard ]] sidinline uint16_t getNormalizedValue ( double value ) const noexcept
	{
		// This function does not get called in real-time, so we can afford to be a bit more accurate
		const auto	tmp = N16 * ( value - vmin ) + rndBuffer[ rndIndex++ & 4095 ];

 		assert ( tmp >= 0.0 && tmp < 65536.0 );
 		return uint16_t ( tmp );
	}

	template<int N>
	[[ nodiscard ]] sidinline uint16_t getNormalizedCurrentFactor ( double wl ) const noexcept
	{
		const auto	tmp = ( 1 << N ) * currFactorCoeff * wl;
		assert ( tmp >= 0.0 && tmp < 65536.0 );
		return uint16_t ( tmp );
	}

	[[ nodiscard ]] sidinline uint16_t getNVmin () const noexcept
	{
		const auto	tmp = N16 * vmin;
		assert ( tmp >= 0.0 && tmp < 65536.0 );
		return uint16_t ( tmp );
	}
};

} // namespace reSIDfp
