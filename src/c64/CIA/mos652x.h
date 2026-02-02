#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2009-2014 VICE Project
* Copyright 2007-2010 Antti Lankila
* Copyright 2000 Simon White
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

#include <memory>

#include <cstdint>

#include "interrupt.h"
#include "timer.h"
#include "tod.h"
#include "../../EventScheduler.h"
#include "SerialPort.h"

class EventContext;

namespace libsidplayfp
{

class MOS652X;

/**
* This is the timer A of this CIA.
*
* @author Ken Händel
*
*/
class TimerA final : public Timer
{
private:
	/**
	* Signal underflows of Timer A to Timer B.
	*/
	void underFlow () noexcept override;

	void serialPort () noexcept override;

public:
	/**
	* Create timer A.
	*/
	TimerA ( EventScheduler& scheduler, MOS652X& _parent ) : Timer ( "CIA Timer A", scheduler, _parent ) {}
};

/**
* This is the timer B of this CIA.
*
* @author Ken Händel
*
*/
class TimerB final : public Timer
{
private:
	void underFlow () noexcept override;

public:
	/**
	* Create timer B.
	*/
	TimerB ( EventScheduler& scheduler, MOS652X& _parent ) : Timer ( "CIA Timer B", scheduler, _parent ) {}

	/**
	* Receive an underflow from Timer A.
	*/
	void cascade () noexcept
	{
		// we pretend that we are CPU doing a write to ctrl register
		syncWithCpu ();
		state |= CIAT_STEP;
		wakeUpAfterSyncWithCpu ();
	}

	/**
	* Check if start flag is set.
	*
	* @return true if start flag is set, false otherwise
	*/
	bool started () const noexcept { return ( state & CIAT_CR_START ) != 0; }
};

/**
* InterruptSource that acts like new CIA
*/
class InterruptSource8521 final : public InterruptSource
{
protected:
	void triggerInterrupt () noexcept override
	{
		idr |= INTERRUPT_REQUEST;
		idrTemp |= INTERRUPT_REQUEST;

		if ( ack0 () )
			scheduleIrq ();
	}

public:
	InterruptSource8521 ( EventScheduler& scheduler, MOS652X& parent ) : InterruptSource ( scheduler, parent )	{}

	void trigger ( uint8_t interruptMask ) noexcept override;
};

/**
* InterruptSource that acts like old CIA
*/
class InterruptSource6526 final : public InterruptSource
{
protected:
	void triggerInterrupt () noexcept override { idr |= INTERRUPT_REQUEST; }

public:
	InterruptSource6526 ( EventScheduler& scheduler, MOS652X& parent ) : InterruptSource ( scheduler, parent )	{}

	void trigger ( uint8_t interruptMask ) noexcept override;

	uint8_t clear () noexcept override;
};

/**
* This class is heavily based on the ciacore/ciatimer source code from VICE.
* The CIA state machine is lifted as-is. Big thanks to VICE project!
* The Serial Port emulation is based on Denise emu code.
*/
class MOS652X
{
	friend class InterruptSource;
	friend class SerialPort;
	friend class TimerA;
	friend class TimerB;
	friend class Tod;

public:
	using model_t = enum : uint8_t
	{
		MOS6526 = 0,     ///< Old CIA model, interrupts are delayed by 1 clock
		MOS8521,        ///< New CIA model
		MOS6526W4485,   ///< A batch of old CIA model with unique serial port behavior
	};

private:
	static const char* credit;

protected:
	/// Event context.
	EventScheduler& eventScheduler;

	/// Ports
	uint8_t&	pra;
	uint8_t&	prb;
	uint8_t&	ddra;
	uint8_t&	ddrb;

	/// These are all CIA registers.
	uint8_t regs[ 0x10 ];

	/// Timers A and B.
	TimerA timerA;
	TimerB timerB;

	/// Interrupt Source
	std::unique_ptr<InterruptSource> interruptSource;

	/// TOD
	Tod tod;

	/// Serial Data Registers
	SerialPort serialPort;

	/// Events
	//@{
	EventCallback<MOS652X> bTickEvent;
	//@}

private:
	/**
	* Trigger an interrupt from TOD.
	*/
	void todInterrupt () noexcept;

	/**
	* Trigger an interrupt from Serial Port.
	*/
	void spInterrupt () noexcept;

	/**
	* This event exists solely to break the ambiguity of what scheduling on
	* top of PHI1 causes, because there is no ordering between events on
	* same phase. Thus it is scheduled in PHI2 to ensure the b.event() is
	* run once before the value changes.
	*
	* - PHI1 a.event() (which calls underFlow())
	* - PHI1 b.event()
	* - PHI2 bTick.event()
	* - PHI1 a.event()
	* - PHI1 b.event()
	*/
	void bTick () noexcept;

	/**
	* Timer A underflow.
	*/
	void underflowA () noexcept;

	/** Timer B underflow. */
	void underflowB () noexcept;

	/**
	* Handle the serial port.
	*/
	void handleSerialPort () noexcept;

protected:
	/**
	* Create a new CIA.
	*
	* @param context the event context
	*/
	MOS652X ( EventScheduler& scheduler );

	/**
	* Signal interrupt.
	*
	* @param state
	*            interrupt state
	*/
	virtual void interrupt ( bool state ) noexcept = 0;

	virtual void portA () noexcept {}
	virtual void portB () noexcept {}

	/**
	* Timers can appear on the port.
	*/
	uint8_t adjustDataPort ( uint8_t data ) noexcept;

	/**
	* Read CIA register.
	*
	* @param addr
	*            register address to read (lowest 4 bits)
	*/
	uint8_t read ( uint8_t addr ) noexcept;

	/**
	* Write CIA register.
	*
	* @param addr
	*            register address to write (lowest 4 bits)
	* @param data
	*            value to write
	*/
	void write ( uint8_t addr, uint8_t data ) noexcept;

public:
	/**
	* Select chip model.
	*
	* @param model
	*/
	void setModel ( model_t model ) noexcept;

	/**
	* Reset CIA.
	*/
	virtual void reset () noexcept;

	/**
	* Get the credits.
	*
	* @return the credits
	*/
	static const char* credits () noexcept;

	/**
	* Set day-of-time event occurence of rate.
	*
	* @param clock
	*/
	void setDayOfTimeRate ( unsigned int clock ) noexcept { tod.setPeriod ( clock ); }
};

}
