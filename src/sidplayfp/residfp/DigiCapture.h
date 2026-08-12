#pragma once
/*
* This file is part of libSidplayEZ, a SID player engine based on libsidplayfp.
*
* Copyright 2026 Michael Hartmann <mike@ultrasid.com>
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

#include <array>
#include <cstdint>

namespace reSIDfp
{

enum class DigiMode : uint8_t;
template <bool> class Voice;

//-----------------------------------------------------------------------------

/**
* Derives the display-ready signed digi sample per output sample from the
* register shadow and live voice state; every tuned table lives in the cpp
*/
class DigiCapture final
{
public:
	DigiCapture ();

	/**
	* Pick how the digi buffer derives its samples; each mode knows the
	* register it reads
	*/
	void setMode ( DigiMode newMode ) noexcept;

	/**
	* Measurement variant: the computed display modes move with any music, so
	* rate detection watches the raw write stream of the technique's register
	*/
	void setScanMode ( DigiMode newMode ) noexcept;

	void setSmoothing ( const bool enable ) noexcept	{	smooth = enable;	}

	void setSamplingRate ( double samplingFrequency ) noexcept;

	/**
	* One signed display-ready sample; mixedSample feeds the output mode
	*/
	template <bool is6581>
	[[ nodiscard ]] int8_t capture ( const uint8_t* lastpoke, const Voice<is6581>* voice, int mixedSample ) noexcept;

	// unknown only: how often each write register ($D400-$D418) changed, per
	// 60 Hz block; busyBlocks counts blocks with digi-rate change density
	static constexpr int	watchedRegs = 25;

	struct WriteRates final
	{
		std::array<uint16_t, watchedRegs>	maxPerBlock {};
		std::array<uint32_t, watchedRegs>	busyBlocks {};
	};

	[[ nodiscard ]] const WriteRates& getWriteRates () const noexcept	{	return rates;	}

private:
	DigiMode	mode {};	// Zero value = nibble
	bool		smooth = true;

	// The tuned corner frequencies as per-sample alphas at the active rate;
	// the mode alpha tracks the current mode
	double	sampleRate = 44100.0;
	float	alpha = 0.0f;
	float	dcAlpha = 0.0f;
	float	carminaAlpha = 0.0f;

	// Conditioning state: a two-pole low-pass and a slow DC blocker that
	// re-centers static levels (idle duty, music instruments sharing the voice)
	float	lp1 = 0.0f;
	float	lp2 = 0.0f;
	float	dc = 0.0f;

	// carmina only: voice 2's pulse-position stream (the choir line)
	// demodulates at its own, much lower corner before joining the sum (the
	// shared corner serves voice 1)
	float	carminaLp = 0.0f;

	// unknown only: the register shadow snapshot the per-sample compare runs
	// against, and the change counters of the running block
	WriteRates	rates;
	std::array<uint8_t, watchedRegs>	prevPoke {};
	std::array<uint16_t, watchedRegs>	blockChanges {};
	int		blockSamples = 735;
	int		blockCountdown = 735;
};

}
