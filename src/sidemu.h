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

#include <string>

#include "sidplayfp/SidConfig.h"
#include "Event.h"
#include "EventScheduler.h"

#include "c64/Banks/Bank.h"

#include "sidplayfp/residfp/SID.h"

#include "EZ/config.h"

namespace libsidplayfp
{

/**
* Inherit this class to create a new SID emulation.
*/
class sidemu final : public Bank
{
private:
	reSIDfp::SID	m_sid;

	uint8_t			lastpoke[ 0x20 ] = {};

public:
	// Bank functions
	sidinline void poke ( uint16_t address, uint8_t value ) override
	{
		lastpoke[ address & 0x1f ] = value;
		write ( address & 0x1f, value );
	}
	sidinline uint8_t peek ( uint16_t address ) override { return read ( address & 0x1f ); }

	void getStatus ( uint8_t regs[ 0x20 ] ) const { std::copy_n ( lastpoke, std::size ( lastpoke ), regs ); }

	/**
	* Buffer size. 5000 is roughly 5 ms at 96 kHz
	*/
	static constexpr auto	OUTPUTBUFFERSIZE = 5000u;

protected:
	EventScheduler&	eventScheduler;

	event_clock_t	m_accessClk = 0;

	// The sample buffer
	int16_t		m_buffer[ OUTPUTBUFFERSIZE ];

	// Current position in buffer
	int	m_bufferpos = 0;

	std::string m_error = "N/A";

public:
	sidemu ( EventScheduler& eventScheduler );

	void reset ( uint8_t volume );

	/**
	* Clock the SID chip
	*/
	sidinline void clock ()
	{
		const event_clock_t	cycles = eventScheduler.getTime ( EVENT_CLOCK_PHI1 ) - m_accessClk;
		m_accessClk += cycles;
		m_bufferpos += m_sid.clock ( (unsigned int)cycles, m_buffer + m_bufferpos );
	}

	/**
	* Set SID model.
	*/
	void model ( SidConfig::sid_model_t _model )	{	m_sid.setChipModel ( _model == SidConfig::MOS6581 ? reSIDfp::MOS6581 : reSIDfp::MOS8580 );	}

	/**
	* Set the sampling method.
	*
	* @param systemfreq
	* @param outputfreq
	*/
	void sampling ( float systemfreq, float outputfreq )	{	m_sid.setSamplingParameters ( systemfreq, outputfreq );		}

	/**
	* Get a detailed error message.
	*/
	[[ nodiscard ]] const char* error () const { return m_error.c_str (); }

	[[ nodiscard ]] sidinline uint8_t read ( uint8_t addr ) 		{	clock ();	return m_sid.read ( addr );	}
	sidinline void write ( uint8_t addr, uint8_t data ) 			{	clock ();	m_sid.write ( addr, data );	}

	void combinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold )	{	m_sid.setCombinedWaveforms ( cws, threshold );	}

	void filter6581Curve ( double filterCurve )			{	m_sid.setFilter6581Curve ( filterCurve );	}
	void filter6581Range ( double adjustment )			{	m_sid.setFilter6581Range ( adjustment );	}
	void filter6581Gain ( double adjustment )			{	m_sid.setFilter6581Gain ( adjustment );		}

	void filter6581Digi ( double adjustment )			{	m_sid.setFilter6581Digi ( adjustment );		}

	void voice6581DCDrift ( double adjustment )			{	m_sid.setVoiceDCDrift ( adjustment );		}

	void filter8580Curve ( double filterCurve )			{	m_sid.setFilter8580Curve ( filterCurve );	}

	void setDacLeakage ( const double leakage )			{	m_sid.setDacLeakage ( leakage );	}

	[[ nodiscard ]] float getInternalEnvValue ( int voiceNo ) const		{	return m_sid.getEnvLevel ( voiceNo );		}

	/**
	* Get the current position in buffer.
	*/
	[[ nodiscard ]] sidinline int bufferpos () const { return m_bufferpos; }

	/**
	* Set the position in buffer.
	*/
	void bufferpos ( int pos ) { m_bufferpos = pos; }

	/**
	* Get the buffer.
	*/
	[[ nodiscard ]] sidinline int16_t* buffer () { return &m_buffer[ 0 ]; }
};

}
