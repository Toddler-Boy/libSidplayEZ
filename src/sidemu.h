#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2019 Leandro Nini <drfiemost@users.sourceforge.net>
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
#include <string>

#include "sidplayfp/SidConfig.h"
#include "Event.h"
#include "EventScheduler.h"

#include "c64/Banks/Bank.h"

#include "sidplayfp/residfp/DigiMode.h"
#include "sidplayfp/residfp/SID.h"

#include "EZ/config.h"

// Build option: 1 (default) compiles in the register write sink, 0 removes it entirely
#ifndef SIDPLAYEZ_WRITE_SINK
	#define SIDPLAYEZ_WRITE_SINK 1
#endif

namespace libsidplayfp
{

#if SIDPLAYEZ_WRITE_SINK
/**
* Register write observer. Called from sidemu::write () after the chip is clocked to
* the write, with the emulated PHI1 cycle of the write. Runs on the emulation thread.
*/
struct WriteSink
{
	using Fn = void ( * ) ( void* user, uint8_t chip, uint8_t reg, uint8_t val, int64_t cycle ) noexcept;

	Fn		fn = nullptr;
	void*	user = nullptr;
};
#endif

class sidemu : public Bank
{
	friend class SaveState;

public:
	// Bank functions
	sidinline void poke ( uint16_t address, uint8_t value ) noexcept override { write ( address & 0x1f, value ); }
	sidinline uint8_t peek ( uint16_t address ) noexcept override { return read ( address & 0x1f ); }

	// The emulation's register shadow, the raw last-written bytes
	virtual void getStatus ( uint8_t regs[ 0x20 ] ) const noexcept = 0;

public:

	/**
	* Buffer size. 5000 is roughly 5 ms at 96 kHz
	*/
	static constexpr auto	OUTPUTBUFFERSIZE = 5000u;

protected:
	EventScheduler& eventScheduler;

	event_clock_t	m_accessClk = 0;

#if SIDPLAYEZ_WRITE_SINK
	// Optional register write observer, unset by default
	WriteSink	m_sink;
	uint8_t		m_chipIndex = 0;
#endif

	// The sample buffer
	int16_t		m_buffer[ OUTPUTBUFFERSIZE ];

	// The digi buffer
	int8_t		m_bufferDigi[ OUTPUTBUFFERSIZE ];

	// Current position in buffer
	int	m_bufferpos = 0;

	std::string m_error = "N/A";

public:
	sidemu ( EventScheduler& _eventScheduler )
		: eventScheduler ( _eventScheduler )
	{
	}

	virtual ~sidemu () = default;

#if SIDPLAYEZ_WRITE_SINK
	void setWriteSink ( const WriteSink& sink, const uint8_t chipIndex ) noexcept
	{
		m_sink = sink;
		m_chipIndex = chipIndex;
	}
#endif

	virtual void reset ( uint8_t /*volume*/ ) noexcept
	{
		m_accessClk = 0;
	}

	/**
	* Clock the SID chip
	*/
	virtual inline void clock () noexcept = 0;

	/**
	* Set the sampling method.
	*
	* @param systemfreq
	* @param outputfreq
	*/
	virtual void sampling ( float systemfreq, float outputfreq ) noexcept = 0;

	/**
	* Get a detailed error message.
	*/
	[[ nodiscard ]] const char* error () const noexcept { return m_error.c_str (); }

	[[ nodiscard ]] virtual sidinline uint8_t read ( uint8_t addr ) noexcept = 0;
	virtual sidinline void write ( uint8_t addr, uint8_t data ) noexcept = 0;

	/**
	* Arm the one-shot start-up declick. Call at the warm-up -> playback boundary.
	*/
	virtual void armStartupDeclick () noexcept {}

	virtual void voice6581CombinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold ) noexcept = 0;
	virtual void voice8580CombinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold ) noexcept = 0;

	// How the digi buffer derives its samples; the mode implies the register
	// its data rides on
	virtual void setDigiCapture ( reSIDfp::DigiMode mode ) noexcept = 0;

	// Measurement variant: the raw write stream of the technique's register
	virtual void setDigiScan ( reSIDfp::DigiMode mode ) noexcept = 0;

	// The unknown scan mode's per-register change counts
	[[ nodiscard ]] virtual const reSIDfp::DigiCapture::WriteRates& getDigiWriteRates () const noexcept = 0;

	virtual void digiSmoothing ( bool enable ) noexcept = 0;

	virtual void filter6581Curve ( double filterCurve ) noexcept = 0;
	virtual void filter6581_uCoxAndCap ( double uCox, bool oldCap ) noexcept = 0;
	virtual void filter6581Gain ( double adjustment ) noexcept = 0;
	virtual void filter6581Saturation ( double saturation ) noexcept = 0;
	virtual void filter6581Resonance ( double resonance ) noexcept = 0;
	virtual void voice6581DCDrift ( double adjustment ) noexcept = 0;
	virtual void voice6581WaveDCOffset ( double adjustment ) noexcept = 0;
	virtual void voice6581DCBias ( double bias ) noexcept = 0;
	virtual void filter6581ExtInDC ( double adjustment ) noexcept = 0;
	virtual void voiceSawPulseUltra ( bool enable ) noexcept = 0;
	virtual void voice6581LeakageRate ( double rate ) noexcept = 0;

	virtual void filter8580Curve ( double filterCurve ) noexcept = 0;

	virtual void setDacLeakage ( const double leakage ) noexcept = 0;
	virtual void externalFilterResistance ( double ohms ) noexcept = 0;

	[[ nodiscard ]] virtual float getInternalEnvValue ( int voiceNo ) const noexcept = 0;

	/**
	* Get the current position in buffer
	*/
	[[ nodiscard ]] sidinline int bufferpos () const noexcept { return m_bufferpos; }

	/**
	* Set the position in buffer
	*/
	void setBufferPos ( int pos ) noexcept { m_bufferpos = pos; }

	/**
	* Get the buffer
	*/
	[[ nodiscard ]] sidinline int16_t* getBuffer () noexcept { return &m_buffer[ 0 ]; }

	/**
	* Get the digi-buffer
	*/
	[[ nodiscard ]] sidinline int8_t* getDigiBuffer () noexcept { return &m_bufferDigi[ 0 ]; }
};
//-----------------------------------------------------------------------------

//
// Inherit this class to create a new SID emulations
//
template <typename FLT>
class sidemuSpec final : public sidemu
{
	friend class SaveState;

public:
	sidemuSpec ( EventScheduler& _eventScheduler )
		: sidemu ( _eventScheduler )
	{
		setDigiCapture ( reSIDfp::DigiMode::nibble );
		reset ( 0xF );
	}

	void reset ( uint8_t volume ) noexcept override
	{
		sidemu::reset ( volume );

		m_sid.reset ();
		m_sid.write ( 0x18, volume );
	}

	sidinline void clock () noexcept override
	{
		const event_clock_t	cycles = eventScheduler.getTime ( EVENT_CLOCK_PHI1 ) - m_accessClk;
		m_accessClk += cycles;

		m_bufferpos += m_sid.clock ( (unsigned int)cycles, m_buffer + m_bufferpos, m_bufferDigi + m_bufferpos );
	}

	void sampling ( float systemfreq, float outputfreq ) noexcept override
	{
		m_sid.setSamplingParameters ( systemfreq, outputfreq );
	}

	[[ nodiscard ]] sidinline uint8_t read ( uint8_t addr ) noexcept override
	{
		clock ();
		return m_sid.read ( addr );
	}

	sidinline void write ( uint8_t addr, uint8_t data ) noexcept override
	{
		clock ();

#if SIDPLAYEZ_WRITE_SINK
		if ( m_sink.fn ) [[ unlikely ]]
			m_sink.fn ( m_sink.user, m_chipIndex, addr, data, m_accessClk );
#endif

		m_sid.write ( addr, data );
	}

	void armStartupDeclick () noexcept override { m_sid.armStartupDeclick (); }

	void voice6581CombinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold ) noexcept override { m_sid.setCombinedWaveforms6581 ( cws, threshold ); }
	void voice8580CombinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold ) noexcept override { m_sid.setCombinedWaveforms8580 ( cws, threshold ); }

	void setDigiCapture ( reSIDfp::DigiMode mode ) noexcept override { m_sid.setDigiCapture ( mode ); }
	void setDigiScan ( reSIDfp::DigiMode mode ) noexcept override { m_sid.setDigiScan ( mode ); }
	[[ nodiscard ]] const reSIDfp::DigiCapture::WriteRates& getDigiWriteRates () const noexcept override { return m_sid.getDigiWriteRates (); }

	void getStatus ( uint8_t regs[ 0x20 ] ) const noexcept override { m_sid.getRegs ( regs ); }

	void digiSmoothing ( bool enable ) noexcept override { m_sid.setDigiSmoothing ( enable ); }

	void filter6581Curve ( double filterCurve ) noexcept override { m_sid.setFilter6581Curve ( filterCurve ); }
	void filter6581_uCoxAndCap ( double uCox, bool oldCap ) noexcept override { m_sid.setFilter6581_uCoxAndCap ( uCox, oldCap ); }
	void filter6581Gain ( double adjustment ) noexcept override { m_sid.setFilter6581Gain ( adjustment ); }
	void filter6581Saturation ( double saturation ) noexcept override { m_sid.setFilter6581Saturation ( saturation ); }
	void filter6581Resonance ( double resonance ) noexcept override { m_sid.setFilter6581Resonance ( resonance ); }
	void voice6581DCDrift ( double adjustment ) noexcept override { m_sid.setVoiceDCDrift ( adjustment ); }
	void voice6581WaveDCOffset ( double adjustment ) noexcept override { m_sid.setWaveDCOffset ( adjustment ); }
	void voice6581DCBias ( double bias ) noexcept override { m_sid.setVoiceDCBias ( bias ); }
	void filter6581ExtInDC ( double adjustment ) noexcept override { m_sid.setFilter6581ExtInDC ( adjustment ); }
	void voiceSawPulseUltra ( bool enable ) noexcept override { m_sid.setSawPulseUltra ( enable ); }
	void voice6581LeakageRate ( double rate ) noexcept override { m_sid.setLeakageRate ( rate ); }
	void filter8580Curve ( double filterCurve ) noexcept override { m_sid.setFilter8580Curve ( filterCurve ); }

	void setDacLeakage ( const double leakage ) noexcept override { m_sid.setDacLeakage ( leakage ); }
	void externalFilterResistance ( double ohms ) noexcept override { m_sid.setExternalFilterResistance ( ohms ); }

	[[ nodiscard ]] float getInternalEnvValue ( int voiceNo ) const noexcept override	{	return m_sid.getEnvLevel ( voiceNo );	}

private:
	reSIDfp::SID<FLT>		m_sid;
};
//-----------------------------------------------------------------------------

}