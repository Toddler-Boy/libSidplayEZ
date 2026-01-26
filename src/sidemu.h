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

/**
* Inherit this class to create a new SID emulation.
*/
class sidemu final : public Bank
{
private:
	reSIDfp::SID<reSIDfp::Filter6581<true>>		m_sid6581;
	reSIDfp::SID<reSIDfp::Filter6581<false>>	m_sid6581_noFilter;
	reSIDfp::SID<reSIDfp::Filter8580<true>>		m_sid8580;
	reSIDfp::SID<reSIDfp::Filter8580<false>>	m_sid8580_noFilter;

	int		modelIndex = 0;

	uint8_t		lastpoke[ 0x20 ] = {};

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
	sidemu ( EventScheduler& _eventScheduler )
		: eventScheduler ( _eventScheduler )
	{
		reset ( 0xF );

		m_sid6581.setChipModel ( reSIDfp::MOS6581 );
		m_sid6581_noFilter.setChipModel ( reSIDfp::MOS6581 );
		m_sid8580.setChipModel ( reSIDfp::MOS8580 );
		m_sid8580_noFilter.setChipModel ( reSIDfp::MOS8580 );
	}

	void reset ( uint8_t volume )
	{
		std::fill ( std::begin ( lastpoke ), std::end ( lastpoke ), 0 );

		m_accessClk = 0;

		m_sid6581.reset ();
		m_sid6581.write ( 0x18, volume );

		m_sid6581_noFilter.reset ();
		m_sid6581_noFilter.write ( 0x18, volume );

		m_sid8580.reset ();
		m_sid8580.write ( 0x18, volume );

		m_sid8580_noFilter.reset ();
		m_sid8580_noFilter.write ( 0x18, volume );
	}

	/**
	* Clock the SID chip
	*/
	sidinline void clock ()
	{
		const event_clock_t	cycles = eventScheduler.getTime ( EVENT_CLOCK_PHI1 ) - m_accessClk;
		m_accessClk += cycles;

		switch ( modelIndex )
		{
			case 0:
				m_bufferpos += m_sid6581.clock ( (unsigned int)cycles, m_buffer + m_bufferpos );
				break;

			case 1:
				m_bufferpos += m_sid6581_noFilter.clock ( (unsigned int)cycles, m_buffer + m_bufferpos );
				break;

			case 2:
				m_bufferpos += m_sid8580.clock ( (unsigned int)cycles, m_buffer + m_bufferpos );
				break;

			case 3:
				m_bufferpos += m_sid8580_noFilter.clock ( (unsigned int)cycles, m_buffer + m_bufferpos );
				break;
		}
	}

	/**
	* Set SID model.
	*/
	void model ( SidConfig::sid_model_t _model, const bool useFilter = true )
	{
		modelIndex = _model == SidConfig::MOS6581 ? 0 : 2;
		if ( ! useFilter )
			++modelIndex;
	}

	/**
	* Set the sampling method.
	*
	* @param systemfreq
	* @param outputfreq
	*/
	void sampling ( float systemfreq, float outputfreq )
	{
		m_sid6581.setSamplingParameters ( systemfreq, outputfreq );
		m_sid6581_noFilter.setSamplingParameters ( systemfreq, outputfreq );
		m_sid8580.setSamplingParameters ( systemfreq, outputfreq );
		m_sid8580_noFilter.setSamplingParameters ( systemfreq, outputfreq );
	}

	/**
	* Get a detailed error message.
	*/
	[[ nodiscard ]] const char* error () const { return m_error.c_str (); }

	[[ nodiscard ]] sidinline uint8_t read ( uint8_t addr )
	{
		clock ();

		switch ( modelIndex )
		{
			case 0:			return m_sid6581.read ( addr );
			case 1:			return m_sid6581_noFilter.read ( addr );
			case 2:			return m_sid8580.read ( addr );
		}
		return m_sid8580_noFilter.read ( addr );
	}

	sidinline void write ( uint8_t addr, uint8_t data )
	{
		clock ();

		switch ( modelIndex )
		{
			case 0:
				m_sid6581.write ( addr, data );
				break;

			case 1:
				m_sid6581_noFilter.write ( addr, data );
				break;

			case 2:
				m_sid8580.write ( addr, data );
				break;

			case 3:
				m_sid8580_noFilter.write ( addr, data );
				break;
		}
	}

	void combinedWaveforms ( reSIDfp::CombinedWaveforms cws, const float threshold )
	{
		m_sid6581.setCombinedWaveforms ( cws, threshold );
		m_sid6581_noFilter.setCombinedWaveforms ( cws, threshold );
		m_sid8580.setCombinedWaveforms ( cws, threshold );
		m_sid8580_noFilter.setCombinedWaveforms ( cws, threshold );
	}

	void filter6581Curve ( double filterCurve )			{	m_sid6581.setFilter6581Curve ( filterCurve );	}
	void filter6581Range ( double adjustment )			{	m_sid6581.setFilter6581Range ( adjustment );	}
	void filter6581Gain ( double adjustment )			{	m_sid6581.setFilter6581Gain ( adjustment );		}

	void filter6581Digi ( double adjustment )
	{
		m_sid6581.setFilter6581Digi ( adjustment );
		m_sid6581_noFilter.setFilter6581Digi ( adjustment );
	}

	void voice6581DCDrift ( double adjustment )
	{
		m_sid6581.setVoiceDCDrift ( adjustment );
		m_sid6581_noFilter.setVoiceDCDrift ( adjustment );
	}

	void filter8580Curve ( double filterCurve )
	{
		m_sid8580.setFilter8580Curve ( filterCurve );
	}

	void setDacLeakage ( const double leakage )
	{
		m_sid6581.setDacLeakage ( leakage );
		m_sid6581_noFilter.setDacLeakage ( leakage );
		m_sid8580.setDacLeakage ( leakage );
		m_sid8580_noFilter.setDacLeakage ( leakage );
	}

	[[ nodiscard ]] float getInternalEnvValue ( int voiceNo ) const
	{
		switch ( modelIndex )
		{
			case 0:			return m_sid6581.getEnvLevel ( voiceNo );
			case 1:			return m_sid6581_noFilter.getEnvLevel ( voiceNo );
			case 2:			return m_sid8580.getEnvLevel ( voiceNo );
		}
		return m_sid8580_noFilter.getEnvLevel ( voiceNo );
	}

	[[ nodiscard ]] bool wasFilterUsed () const
	{
		switch ( modelIndex )
		{
			case 0:			return m_sid6581.wasFilterUsed ();
			case 1:			return m_sid6581_noFilter.wasFilterUsed ();
			case 2:			return m_sid8580.wasFilterUsed ();
		}
		return m_sid8580_noFilter.wasFilterUsed ();
	}

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
