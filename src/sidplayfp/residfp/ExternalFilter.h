#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2004 Dag Lem <resid@nimrod.no>
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

namespace libsidplayfp { class SaveState; }

namespace reSIDfp
{

/**
* The audio output stage in a Commodore 64 consists of two STC networks, a
* low-pass RC filter with 3 dB frequency 16kHz followed by a DC-blocker which
* acts as a high-pass filter with a cutoff dependent on the attached audio
* equipment impedance. Here we suppose an impedance of 10kOhm resulting
* in a 3 dB attenuation at 1.6Hz.
*
* ~~~
*                                 9/12V
* -----+
* audio|       10k                  |
*      +---o----R---o--------o-----(K)          +-----
*  out |   |        |        |      |           |audio
* -----+   R 1k     C 1000   |      |    10 uF  |
*          |        |  pF    +-C----o-----C-----+ 10k
*                             470   |           |
*         GND      GND         pF   R 1K        | amp
*          *                   **   |           +-----
*
*                                  GND
* ~~~
*
* The STC networks are connected with a [BJT] based [common collector]
* used as a voltage follower (featuring a 2SC1815 NPN transistor).
*
* * To operate properly the 6581 audio output needs a pull-down resistor
*   (1KOhm recommended, not needed on 8580)
* ** The C64c board additionally includes a [bootstrap] condenser to increase
*    the input impedance of the common collector.
*
* [BJT]: https://en.wikipedia.org/wiki/Bipolar_junction_transistor
* [common collector]: https://en.wikipedia.org/wiki/Common_collector
* [bootstrap]: https://en.wikipedia.org/wiki/Bootstrapping_(electronics)
*/
class ExternalFilter final
{
	friend class libsidplayfp::SaveState;

private:
	// Lowpass filter voltage
	int Vlp = 0;

	// Highpass filter voltage
	int Vhp = 0;

	int w0lp_1_s7 = 0;
	int w0hp_1_s17 = 0;

	// Load impedance of the attached audio equipment, which sets the DC-blocker
	// cutoff: 1k (low-impedance load) = 15.9 Hz, 10k (a line-in) = 1.6 Hz
	double	extResistance = 1e3;

	// Clock period, kept so a resistance change can rebuild the coefficients
	double	dt = 1.0e-6;

	void updateCoefficients () noexcept
	{
		// Low-pass:  R = 10kOhm, C = 1000pF; cutoff 1/2*PI*RC = 15915.5 Hz
		w0lp_1_s7 = static_cast<int32_t>( ( dt / ( dt + getRC ( 10e3, 1000e-12 ) ) ) * ( 1 << 7 ) + 0.5 );

		// High-pass: C = 10uF into the external load resistance
		w0hp_1_s17 = static_cast<int32_t>( ( dt / ( dt + getRC ( extResistance, 10e-6 ) ) ) * ( 1 << 17 ) + 0.5 );
	}

public:
	/**
	* SID clocking
	*
	* @param input
	*/
	[[ nodiscard ]] sidinline int clock ( int input ) noexcept
	{
		const auto	Vi = input << 11;
		const auto	dVlp = ( w0lp_1_s7 * ( Vi - Vlp ) >> 7 );
		const auto	dVhp = ( w0hp_1_s17 * ( Vlp - Vhp ) >> 17 );
		Vlp += dVlp;
		Vhp += dVhp;

		return ( Vlp - Vhp ) >> 11;
	}

	constexpr sidinline double getRC ( double res, double cap )
	{
		return res * cap;
	}

	/**
	* Constructor
	*/
	ExternalFilter ()
	{
		reset ();
	}

	/**
	* Setup of the external filter sampling parameters
	*
	* @param frequency the main system clock frequency
	*/
	void setClockFrequency ( double frequency ) noexcept
	{
		dt = 1.0 / frequency;
		updateCoefficients ();
		reset ();
	}

	/**
	* Set the load impedance of the attached audio equipment, which the
	* DC-blocker cutoff depends on: 1 kOhm (default) = 15.9 Hz, 10 kOhm
	* (a line-in) = 1.6 Hz. Click-free, the filter state is kept.
	*
	* @param ohms the load resistance
	*/
	void setResistance ( double ohms ) noexcept
	{
		extResistance = ohms;
		updateCoefficients ();
	}

	/**
	* SID reset
	*/
	void reset () noexcept
	{
		Vlp = Vhp = 0;
	}

	/**
	* Snap the filter state to a given input level, producing no DC transient.
	*
	* The high-pass (DC-blocker) stage otherwise rings for hundreds of ms when the
	* input DC steps abruptly - e.g. when a tune's first play routine sets the master
	* volume from 0 to 15, producing an audible start-up pop. Re-seating Vlp and Vhp
	* at the new operating point keeps the output continuous with zero transient.
	* Only the DC state is touched; AC content resumes normally on the next sample.
	*
	* @param input the current input sample (same units as clock())
	*/
	void settle ( int input ) noexcept
	{
		Vlp = Vhp = input << 11;
	}
};

} // namespace reSIDfp
