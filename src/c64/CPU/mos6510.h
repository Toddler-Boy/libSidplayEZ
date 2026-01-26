#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2019 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <cstdint>

#include "../c64cpu.h"

#include "flags.h"
#include "../../EventCallback.h"
#include "../../EventScheduler.h"

class EventContext;

namespace libsidplayfp
{

/**
* Cycle-exact 6502/6510 emulation core.
*
* Code is based on work by Simon A. White <sidplay2@yahoo.com>.
* Original Java port by Ken Händel. Later on, it has been hacked to
* improve compatibility with Lorenz suite on VICE's test suite.
*
* @author alankila
*/
class MOS6510
{
public:
	class haltInstruction {};

private:
	/**
	* IRQ/NMI magic limit values.
	* Need to be larger than about 0x103 << 3,
	* but can't be min/max for Integer type.
	*/
	static constexpr int MAX = 65536;

	/// Stack page location
	static constexpr uint8_t SP_PAGE = 0x01;

public:
	/// Status register interrupt bit.
	static constexpr int SR_INTERRUPT = 2;

private:
	struct ProcessorCycle
	{
		void ( *func )( MOS6510& ) = nullptr;
		bool nosteal = false;
	};

private:
	/// Event scheduler
	EventScheduler& eventScheduler;

	/// Data bus
	c64cpubus&		dataBus;

	/// Current instruction and subcycle within instruction
	int cycleCount;

	/// When IRQ was triggered. -MAX means "during some previous instruction", MAX means "no IRQ"
	int interruptCycle;

	/// IRQ asserted on CPU
	bool irqAssertedOnPin;

	/// NMI requested?
	bool nmiFlag;

	/// RST requested?
	bool rstFlag;

	/// RDY pin state (stop CPU on read)
	bool rdy;

	/// Address Low summer carry
	bool adl_carry;

	bool d1x1;

	/// The RDY pin state during last throw away read.
	bool rdyOnThrowAwayRead;

	/// Status register
	Flags flags;

	// Data regarding current instruction
	uint16_t Register_ProgramCounter;
	uint16_t Cycle_EffectiveAddress;
	uint16_t Cycle_Pointer;

	uint8_t Cycle_Data;
	uint8_t Register_StackPointer;
	uint8_t Register_Accumulator;
	uint8_t Register_X;
	uint8_t Register_Y;

	/// Table of CPU opcode implementations
	struct ProcessorCycle instrTable[ 0x101 << 3 ] = {};

private:
	void eventWithoutSteals ();
	void eventWithSteals ();
	void removeIRQ ();

	// Represents an instruction subcycle that writes
	FastEventCallback<MOS6510, &MOS6510::eventWithoutSteals>	m_nosteal;
	// Represents an instruction subcycle that reads
	FastEventCallback<MOS6510, &MOS6510::eventWithSteals>		m_steal;

	FastEventCallback<MOS6510, &MOS6510::removeIRQ>				clearInt;

	sidinline void Initialise ();

	// Declare Interrupt Routines
	sidinline void IRQLoRequest ();
	sidinline void IRQHiRequest ();
	sidinline void interruptsAndNextOpcode ();
	sidinline void calculateInterruptTriggerCycle ();

	// Declare Instruction Routines
	sidinline void fetchNextOpcode ();
	sidinline void throwAwayFetch ();
	sidinline void throwAwayRead ();
	sidinline void FetchDataByte ();
	sidinline void FetchLowAddr ();
	sidinline void FetchLowAddrX ();
	sidinline void FetchLowAddrY ();
	sidinline void FetchHighAddr ();
	sidinline void FetchHighAddrX ();
	sidinline void FetchHighAddrX2 ();
	sidinline void FetchHighAddrY ();
	sidinline void FetchHighAddrY2 ();
	sidinline void FetchLowEffAddr ();
	sidinline void FetchHighEffAddr ();
	sidinline void FetchHighEffAddrY ();
	sidinline void FetchHighEffAddrY2 ();
	sidinline void FetchLowPointer ();
	sidinline void FetchLowPointerX ();
	sidinline void FetchHighPointer ();
	sidinline void FetchEffAddrDataByte ();
	sidinline void PutEffAddrDataByte ();
	sidinline void PushLowPC ();
	sidinline void PushHighPC ();
	sidinline void PushSR ();
	sidinline void PopLowPC ();
	sidinline void PopHighPC ();
	sidinline void PopSR ();
	sidinline void brkPushLowPC ();
	sidinline void WasteCycle ();

	sidinline void Push ( uint8_t data );
	sidinline uint8_t Pop ();
	sidinline void compare ( uint8_t data );

	// Delcare Instruction Operation Routines
	sidinline void adc_instr ();
	sidinline void alr_instr ();
	sidinline void anc_instr ();
	sidinline void and_instr ();
	sidinline void ane_instr ();
	sidinline void arr_instr ();
	sidinline void asl_instr ();
	sidinline void asla_instr ();
	sidinline void aso_instr ();
	sidinline void axa_instr ();
	sidinline void axs_instr ();
	sidinline void bcc_instr ();
	sidinline void bcs_instr ();
	sidinline void beq_instr ();
	sidinline void bit_instr ();
	sidinline void bmi_instr ();
	sidinline void bne_instr ();
	sidinline void branch_instr ( bool condition );
	sidinline void fix_branch ();
	sidinline void bpl_instr ();
	sidinline void bvc_instr ();
	sidinline void bvs_instr ();
	sidinline void clc_instr ();
	sidinline void cld_instr ();
	sidinline void cli_instr ();
	sidinline void clv_instr ();
	sidinline void cmp_instr ();
	sidinline void cpx_instr ();
	sidinline void cpy_instr ();
	sidinline void dcm_instr ();
	sidinline void dec_instr ();
	sidinline void dex_instr ();
	sidinline void dey_instr ();
	sidinline void eor_instr ();
	sidinline void inc_instr ();
	sidinline void ins_instr ();
	sidinline void inx_instr ();
	sidinline void iny_instr ();
	sidinline void jmp_instr ();
	sidinline void las_instr ();
	sidinline void lax_instr ();
	sidinline void lda_instr ();
	sidinline void ldx_instr ();
	sidinline void ldy_instr ();
	sidinline void lse_instr ();
	sidinline void lsr_instr ();
	sidinline void lsra_instr ();
	sidinline void oal_instr ();
	sidinline void ora_instr ();
	sidinline void pha_instr ();
	sidinline void pla_instr ();
	sidinline void rla_instr ();
	sidinline void rol_instr ();
	sidinline void rola_instr ();
	sidinline void ror_instr ();
	sidinline void rora_instr ();
	sidinline void rra_instr ();
	sidinline void rti_instr ();
	sidinline void rts_instr ();
	sidinline void sbx_instr ();
	sidinline void say_instr ();
	sidinline void sbc_instr ();
	sidinline void sec_instr ();
	sidinline void sed_instr ();
	sidinline void sei_instr ();
	sidinline void shs_instr ();
	sidinline void sta_instr ();
	sidinline void stx_instr ();
	sidinline void sty_instr ();
	sidinline void tax_instr ();
	sidinline void tay_instr ();
	sidinline void tsx_instr ();
	sidinline void txa_instr ();
	sidinline void txs_instr ();
	sidinline void tya_instr ();
	sidinline void xas_instr ();
	sidinline void sh_instr ();

	/**
	* @throws haltInstruction
	*/
	void invalidOpcode ();

	// Declare Arithmetic Operations
	sidinline void doADC ();
	sidinline void doSBC ();

	sidinline bool checkInterrupts () const { return rstFlag || nmiFlag || ( irqAssertedOnPin && ! flags.getI () ); }

	sidinline void buildInstructionTable ();

public:
	MOS6510 ( EventScheduler& scheduler, c64cpubus& bus );
	~MOS6510 () = default;

	/**
	* Get data from system environment.
	*
	* @param address
	* @return data byte CPU requested
	*/
	sidinline uint8_t cpuRead ( uint16_t addr )					{	return dataBus.cpuRead ( addr );	}

	/**
	* Write data to system environment.
	*
	* @param address
	* @param data
	*/
	sidinline void cpuWrite ( uint16_t addr, uint8_t data )		{	dataBus.cpuWrite ( addr, data );	}

	void reset ();

	static const char* credits ();

	void setRDY ( bool newRDY );

	// Non-standard functions
	void triggerRST ();
	void triggerNMI ();
	void triggerIRQ ();
	void clearIRQ ();
};

}
