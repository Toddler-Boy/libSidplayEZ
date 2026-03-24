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

#include "sidplayfp/residfp/SID.h"

#include "EZ/config.h"

namespace libsidplayfp
{

class sidemu : public Bank
{
private:
	uint8_t		lastpoke[ 0x20 ] = {};

public:
	// Bank functions
	sidinline void poke ( uint16_t address, uint8_t value ) noexcept override
	{
		lastpoke[ address & 0x1f ] = value;
		write ( address & 0x1f, value );
	}
	sidinline uint8_t peek ( uint16_t address ) noexcept override { return read ( address & 0x1f ); }

	void getStatus ( uint8_t regs[ 0x20 ] ) const noexcept { std::copy_n ( lastpoke, std::size ( lastpoke ), regs ); }

	/**
	* Buffer size. 5000 is roughly 5 ms at 96 kHz
	*/
	static constexpr auto	OUTPUTBUFFERSIZE = 5000u;

protected:
	EventScheduler& eventScheduler;

	event_clock_t	m_accessClk = 0;

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

	virtual void reset ( uint8_t /*volume*/ ) noexcept
	{
		std::fill ( std::begin ( lastpoke ), std::end ( lastpoke ), 0 );

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

	virtual void combinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold ) noexcept = 0;

	virtual void filter6581Curve ( double filterCurve ) noexcept = 0;
	virtual void filter6581_uCoxAndCap ( double uCox, bool oldCap ) noexcept = 0;
	virtual void filter6581Gain ( double adjustment ) noexcept = 0;
	virtual void filter6581Digi ( double adjustment ) noexcept = 0;
	virtual void voice6581DCDrift ( double adjustment ) noexcept = 0;
	virtual void voiceSawPulseUltra ( bool enable ) noexcept = 0;

	virtual void filter8580Curve ( double filterCurve ) noexcept = 0;

	virtual void setDacLeakage ( const double leakage ) noexcept = 0;

	[[ nodiscard ]] virtual float getInternalEnvValue ( int voiceNo ) const noexcept = 0;
	[[ nodiscard ]] virtual bool wasFilterUsed () const noexcept = 0;

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
public:
	sidemuSpec ( EventScheduler& _eventScheduler )
		: sidemu ( _eventScheduler )
	{
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
		m_sid.write ( addr, data );
	}

	void combinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold ) noexcept override
	{
		m_sid.setCombinedWaveforms ( cws, threshold );
	}

	void filter6581Curve ( double filterCurve ) noexcept override { m_sid.setFilter6581Curve ( filterCurve ); }
	void filter6581_uCoxAndCap ( double uCox, bool oldCap ) noexcept override { m_sid.setFilter6581_uCoxAndCap ( uCox, oldCap ); }
	void filter6581Gain ( double adjustment ) noexcept override { m_sid.setFilter6581Gain ( adjustment ); }
	void filter6581Digi ( double adjustment ) noexcept override	{	m_sid.setFilter6581Digi ( adjustment );	}
	void voice6581DCDrift ( double adjustment ) noexcept override { m_sid.setVoiceDCDrift ( adjustment ); }
	void voiceSawPulseUltra ( bool enable ) noexcept override { m_sid.setSawPulseUltra ( enable ); }
	void filter8580Curve ( double filterCurve ) noexcept override { m_sid.setFilter8580Curve ( filterCurve ); }

	void setDacLeakage ( const double leakage ) noexcept override { m_sid.setDacLeakage ( leakage ); }

	[[ nodiscard ]] float getInternalEnvValue ( int voiceNo ) const noexcept override	{	return m_sid.getEnvLevel ( voiceNo );	}
	[[ nodiscard ]] bool wasFilterUsed () const noexcept override	{		return m_sid.wasFilterUsed ();	}

private:
	reSIDfp::SID<FLT>		m_sid;
};
//-----------------------------------------------------------------------------

}