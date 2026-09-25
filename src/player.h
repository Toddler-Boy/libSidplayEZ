#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2022 Leandro Nini <drfiemost@users.sourceforge.net>
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
#include <span>

#include "sidplayfp/SidConfig.h"
#include "sidplayfp/SidTune.h"
#include "SidInfoImpl.h"
#include "sidemu.h"

#include "mixer.h"
#include "c64/c64.h"

#include <vector>

namespace libsidplayfp
{

class Player final
{
	friend class SaveState;

private:
	c64			m_c64;				// Commodore 64 emulator
	Mixer		m_mixer;			// Mixer
	SidTune*	m_tune = nullptr;	// Emulator info
	SidInfoImpl	m_info;				// Tune info
	SidConfig	m_cfg;				// User Configuration Settings

	std::vector<sidemu*>	m_sidEmu;	// emulation of the actual SID chips, as many as the tune asks for

#if SIDPLAYEZ_WRITE_SINK
	WriteSink	m_writeSink;			// Register write observer, re-applied to every chip sidCreate () makes
#endif

	std::string	m_errorString = "N/A";

	uint32_t	m_startTime = 0;
	uint8_t		videoSwitch;					// PAL/NTSC switch value

	// RAM and color RAM as initialise () left them, the baseline a save-state
	// stores its RAM changes against
	std::vector<uint8_t>	m_stateReference;

	/**
	* Get the C64 model for the current loaded tune.
	*
	* @param defaultModel the default model
	* @param forced true if the default model should be forced in spite of tune model
	*/
	c64::model_t c64model ( SidConfig::c64_model_t defaultModel, bool forced );

	// False when the tune cannot be set up, with the reason in error ()
	[[ nodiscard ]] bool initialise ();

	void sidRelease ();
	void sidCreate ( SidConfig::sid_model_t defaultModel, bool forced, const std::vector<uint16_t>& sidAddresses, const bool useFilter );

	void sidDestroy ();

	void sidParams ( double cpuFreq, int frequency );

	sidinline void run ( unsigned int events )
	{
		while ( events-- )
			m_c64.clock ();
	}

public:
	Player ();
	~Player ();

	bool setConfig ( const SidConfig& cfg, bool force = false );
	[[ nodiscard ]] const SidConfig& getConfig () const { return m_cfg; }

	[[ nodiscard ]] const SidInfo& getInfo () const { return m_info; }

	bool loadTune ( SidTune* tune );
	uint32_t play ( std::span<float> bufferL, std::span<float> bufferR, std::span<const std::span<int8_t>> digiBuffers );

	[[ nodiscard ]] int getNumChips () const { return m_mixer.getNumChips (); }

	/**
	* Check whether an illegal opcode has halted the CPU. Poll after loading a tune
	* and after every play (), which both stop early once it is set.
	*/
	[[ nodiscard ]] bool isJammed () const { return m_c64.isJammed (); }

	void set6581CombinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold );
	void set8580CombinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold );

	void setDigiCapture ( const reSIDfp::DigiMode mode );
	void setDigiScan ( const reSIDfp::DigiMode mode );

	void setDigiSmoothing ( const bool enable );

	void set6581FilterCurve ( const double value );
	void set6581Filter_uCoxAndCap ( const double uCox, const bool oldCap );
	void set6581FilterGain ( const double value );
	void set6581FilterSaturation ( const double value );
	void set6581FilterResonance ( const double value );

	void setDacLeakage ( const double value );
	void setExternalFilterResistance ( const double ohms );
	void set6581VoiceDCDrift ( const double value );
	void set6581WaveDCOffset ( const double value );
	void set6581VoiceDCBias ( const double value );
	void set6581ExtInDC ( const double value );
	void set6581SawPulseUltra ( const bool enable );
	void set6581LeakageRate ( const double value );

	// Seed for the CIAs' randomized power-on TOD state, applied on the next tune
	// (re)init; zero (the default) keeps the fixed clock, so renders stay deterministic
	void setTodPowerOnSeed ( const uint32_t seed ) { m_c64.setTodPowerOnSeed ( seed ); }

	[[ nodiscard ]] uint32_t timeMs () const { return m_c64.getTimeMs () - m_startTime; }				// Time in milliseconds

	[[ nodiscard ]] const char* error () const { return m_errorString.c_str (); }

	void setKernal ( const uint8_t* rom );
	void setBasic ( const uint8_t* rom );
	void setChargen ( const uint8_t* rom );

	[[ nodiscard ]] uint16_t getCia1TimerA () const { return m_c64.getCia1TimerA (); }

	bool getSidStatus ( int sidNum, uint8_t regs[ 32 ] );
	bool getDigiWriteRates ( int sidNum, reSIDfp::DigiCapture::WriteRates& rates );

	[[ nodiscard ]] uint16_t getInterruptCycles () const { return m_c64.getInterruptCycles (); }

#if SIDPLAYEZ_WRITE_SINK
	/**
	* Observe every SID register write as (chip, reg, val, cycle). Applies to the current chips
	* and survives their recreation. Pass a default WriteSink to detach. The callback runs on
	* the emulation thread inside play (), and must be cheap and non-blocking.
	*/
	void setWriteSink ( const WriteSink& sink );

	// Emulated PHI1 cycles since reset, the time base of the write sink's cycle argument
	[[ nodiscard ]] int64_t getCycleTime () { return m_c64.getEventScheduler ().getTime ( EVENT_CLOCK_PHI1 ); }

	// CPU clock of the loaded tune's C64 model in Hz
	[[ nodiscard ]] double getCpuFrequency () const { return m_c64.getMainCpuSpeed (); }
#endif

	// The complete emulation state at a play () boundary, restorable into a
	// player that loaded and initialised the same tune with the same config
	[[ nodiscard ]] bool saveState ( std::vector<uint8_t>& out );
	[[ nodiscard ]] bool restoreState ( std::span<const uint8_t> in );
};

}
