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

#include "Spline.h"
#include "OpAmp.h"

namespace reSIDfp
{

class FilterModelConfig
{
protected:
	// Capacitor value.
	const double C;

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
	double uCox;				//< Transconductance coefficient: u*Cox
	//@}

	// Derived stuff
	const double vmin, vmax;
	const double denorm, norm;

	// Fixed point scaling for 16 bit op-amp output.
	const double N16;

	const double voice_voltage_range;

	// Current factor coefficient for op-amp integrators
	double currFactorCoeff;

	// Lookup tables for gain and summer op-amps in output stage / filter
	//@{
	uint16_t	mixer0[ 1 ];				//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	mixer1[ 1 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	mixer2[ 2 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	mixer3[ 3 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	mixer4[ 4 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	mixer5[ 5 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	mixer6[ 6 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	mixer7[ 7 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor

	uint16_t*	mixer[ 8 ] = { mixer0, mixer1, mixer2, mixer3, mixer4, mixer5, mixer6, mixer7 };

	uint16_t	summer2[ 2 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	summer3[ 3 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	summer4[ 4 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	summer5[ 5 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	summer6[ 6 << 16 ];			//-V730_NOINIT this is initialized in the derived class constructor

	uint16_t*	summer[ 5 ] = { summer2, summer3, summer4, summer5, summer6 };

	uint16_t	volume[ 16 ][1 << 16];		//-V730_NOINIT this is initialized in the derived class constructor
	uint16_t	resonance[ 16 ][ 1 << 16];	//-V730_NOINIT this is initialized in the derived class constructor
	//@}

	// Reverse op-amp transfer function
	uint16_t	opamp_rev[ 1 << 16 ];	//-V730_NOINIT this is initialized in the derived class constructor

private:
 	double			rndBuffer[ 4096 ];
 	mutable int		rndIndex = 0;

	FilterModelConfig ( const FilterModelConfig& ) = delete;
	FilterModelConfig& operator= ( const FilterModelConfig& ) = delete;

protected:
	/**
	* @param vvr voice voltage range
	* @param vdv voice DC voltage
	* @param c   capacitor value
	* @param vdd Vdd
	* @param vth threshold voltage
	* @param ucox u*Cox
	* @param ominv opamp min voltage
	* @param omaxv opamp max voltage
	*/
	FilterModelConfig ( double vvr, double c, double vdd, double vth, double ucox, const Spline::Point* opamp_voltage, int opamp_size );
	~FilterModelConfig () = default;

	void setUCox ( double new_uCox );

	void buildSummerTable ( OpAmp& opAmp );
	void buildMixerTable ( OpAmp& opampModel, double nRatio );
	void buildVolumeTable ( OpAmp& opampModel, double nDivisor );
	void buildResonanceTable ( OpAmp& opampModel, const double resonance_n[ 16 ] );

public:
	[[ nodiscard ]] uint16_t* getVolume () { return &volume[ 0 ][ 0 ]; }
	[[ nodiscard ]] uint16_t* getResonance () { return &resonance[ 0 ][ 0 ]; }
	[[ nodiscard ]] uint16_t** getSummer () { return summer; }
	[[ nodiscard ]] uint16_t** getMixer () { return mixer; }

	void setVoiceDCVoltage ( double voltage );

	[[ nodiscard ]] sidinline uint16_t getOpampRev ( int i ) const { return opamp_rev[ i ]; }
	[[ nodiscard ]] sidinline double getVddt () const { return Vddt; }
	[[ nodiscard ]] sidinline double getVth () const { return Vth; }

	// helper functions
	[[ nodiscard ]] sidinline uint16_t getNormalizedValue ( double value ) const
	{
		// This function does not get called in real-time, so we can afford to be a bit more accurate
		const auto	tmp = N16 * ( value - vmin ) + rndBuffer[ rndIndex++ & 4095 ];

 		assert ( tmp >= 0.0 && tmp < 65536.0 );
 		return uint16_t ( tmp );
	}

	template<int N> 
	[[ nodiscard ]] sidinline uint16_t getNormalizedCurrentFactor ( double wl ) const
	{
		const auto	tmp = ( 1 << N ) * currFactorCoeff * wl;
		assert ( tmp >= 0.0 && tmp < 65536.0 );
		return uint16_t ( tmp );
	}

	[[ nodiscard ]] sidinline uint16_t getNVmin () const
	{
		const auto	tmp = N16 * vmin;
		assert ( tmp >= 0.0 && tmp < 65536.0 );
		return uint16_t ( tmp );
	}
};

} // namespace reSIDfp
