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

	// Status register interrupt bit
	static constexpr int SR_INTERRUPT = 2;

private:
	/**
	* IRQ/NMI magic limit values.
	* Need to be larger than about 0x103 << 3,
	* but can't be min/max for Integer type.
	*/
	static constexpr int MAX = 65536;

	// Stack page location
	static constexpr uint8_t SP_PAGE = 0x01;

	struct ProcessorCycle
	{
		void ( *func )( MOS6510& ) = nullptr;
		bool nosteal = false;
	};

	// Event scheduler
	EventScheduler& eventScheduler;

	// Data bus
	c64cpubus&		dataBus;

	// Current instruction and subcycle within instruction
	int cycleCount;

	// When IRQ was triggered. -MAX means "during some previous instruction", MAX means "no IRQ"
	int interruptCycle;

	// IRQ asserted on CPU
	bool irqAssertedOnPin;

	// NMI requested?
	bool nmiFlag;

	// RST requested?
	bool rstFlag;

	// RDY pin state (stop CPU on read)
	bool rdy;

	// Address Low summer carry
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

	void eventWithoutSteals () noexcept;
	void eventWithSteals () noexcept;
	void removeIRQ () noexcept;

	// Represents an instruction subcycle that writes
	FastEventCallback<MOS6510, &MOS6510::eventWithoutSteals>	m_nosteal;
	// Represents an instruction subcycle that reads
	FastEventCallback<MOS6510, &MOS6510::eventWithSteals>		m_steal;

	FastEventCallback<MOS6510, &MOS6510::removeIRQ>				clearInt;

	sidinline void Initialise () noexcept;

	// Declare Interrupt Routines
	sidinline void IRQLoRequest () noexcept;
	sidinline void IRQHiRequest () noexcept;
	sidinline void interruptsAndNextOpcode () noexcept;
	sidinline void calculateInterruptTriggerCycle () noexcept;

	// Declare Instruction Routines
	sidinline void fetchNextOpcode () noexcept;
	sidinline void throwAwayFetch () noexcept;
	sidinline void throwAwayRead () noexcept;
	sidinline void FetchDataByte () noexcept;
	sidinline void FetchLowAddr () noexcept;
	sidinline void FetchLowAddrX () noexcept;
	sidinline void FetchLowAddrY () noexcept;
	sidinline void FetchHighAddr () noexcept;
	sidinline void FetchHighAddrX () noexcept;
	sidinline void FetchHighAddrX2 () noexcept;
	sidinline void FetchHighAddrY () noexcept;
	sidinline void FetchHighAddrY2 () noexcept;
	sidinline void FetchLowEffAddr () noexcept;
	sidinline void FetchHighEffAddr () noexcept;
	sidinline void FetchHighEffAddrY () noexcept;
	sidinline void FetchHighEffAddrY2 () noexcept;
	sidinline void FetchLowPointer () noexcept;
	sidinline void FetchLowPointerX () noexcept;
	sidinline void FetchHighPointer () noexcept;
	sidinline void FetchEffAddrDataByte () noexcept;
	sidinline void PutEffAddrDataByte () noexcept;
	sidinline void PushLowPC () noexcept;
	sidinline void PushHighPC () noexcept;
	sidinline void PushSR () noexcept;
	sidinline void PopLowPC () noexcept;
	sidinline void PopHighPC () noexcept;
	sidinline void PopSR () noexcept;
	sidinline void brkPushLowPC () noexcept;
	sidinline void WasteCycle () noexcept;

	sidinline void Push ( uint8_t data ) noexcept;
	sidinline uint8_t Pop () noexcept;
	sidinline void compare ( uint8_t data ) noexcept;

	// Delcare Instruction Operation Routines
	sidinline void adc_instr () noexcept;
	sidinline void alr_instr () noexcept;
	sidinline void anc_instr () noexcept;
	sidinline void and_instr () noexcept;
	sidinline void ane_instr () noexcept;
	sidinline void arr_instr () noexcept;
	sidinline void asl_instr () noexcept;
	sidinline void asla_instr () noexcept;
	sidinline void aso_instr () noexcept;
	sidinline void axa_instr () noexcept;
	sidinline void axs_instr () noexcept;
	sidinline void bcc_instr () noexcept;
	sidinline void bcs_instr () noexcept;
	sidinline void beq_instr () noexcept;
	sidinline void bit_instr () noexcept;
	sidinline void bmi_instr () noexcept;
	sidinline void bne_instr () noexcept;
	sidinline void branch_instr ( bool condition ) noexcept;
	sidinline void fix_branch () noexcept;
	sidinline void bpl_instr () noexcept;
	sidinline void bvc_instr () noexcept;
	sidinline void bvs_instr () noexcept;
	sidinline void clc_instr () noexcept;
	sidinline void cld_instr () noexcept;
	sidinline void cli_instr () noexcept;
	sidinline void clv_instr () noexcept;
	sidinline void cmp_instr () noexcept;
	sidinline void cpx_instr () noexcept;
	sidinline void cpy_instr () noexcept;
	sidinline void dcm_instr () noexcept;
	sidinline void dec_instr () noexcept;
	sidinline void dex_instr () noexcept;
	sidinline void dey_instr () noexcept;
	sidinline void eor_instr () noexcept;
	sidinline void inc_instr () noexcept;
	sidinline void ins_instr () noexcept;
	sidinline void inx_instr () noexcept;
	sidinline void iny_instr () noexcept;
	sidinline void jmp_instr () noexcept;
	sidinline void las_instr () noexcept;
	sidinline void lax_instr () noexcept;
	sidinline void lda_instr () noexcept;
	sidinline void ldx_instr () noexcept;
	sidinline void ldy_instr () noexcept;
	sidinline void lse_instr () noexcept;
	sidinline void lsr_instr () noexcept;
	sidinline void lsra_instr () noexcept;
	sidinline void oal_instr () noexcept;
	sidinline void ora_instr () noexcept;
	sidinline void pha_instr () noexcept;
	sidinline void pla_instr () noexcept;
	sidinline void rla_instr () noexcept;
	sidinline void rol_instr () noexcept;
	sidinline void rola_instr () noexcept;
	sidinline void ror_instr () noexcept;
	sidinline void rora_instr () noexcept;
	sidinline void rra_instr () noexcept;
	sidinline void rti_instr () noexcept;
	sidinline void rts_instr () noexcept;
	sidinline void sbx_instr () noexcept;
	sidinline void say_instr () noexcept;
	sidinline void sbc_instr () noexcept;
	sidinline void sec_instr () noexcept;
	sidinline void sed_instr () noexcept;
	sidinline void sei_instr () noexcept;
	sidinline void shs_instr () noexcept;
	sidinline void sta_instr () noexcept;
	sidinline void stx_instr () noexcept;
	sidinline void sty_instr () noexcept;
	sidinline void tax_instr () noexcept;
	sidinline void tay_instr () noexcept;
	sidinline void tsx_instr () noexcept;
	sidinline void txa_instr () noexcept;
	sidinline void txs_instr () noexcept;
	sidinline void tya_instr () noexcept;
	sidinline void xas_instr () noexcept;
	sidinline void sh_instr () noexcept;

	/**
	* @throws haltInstruction
	*/
	void invalidOpcode ();

	// Declare Arithmetic Operations
	sidinline void doADC () noexcept;
	sidinline void doSBC () noexcept;

	sidinline bool checkInterrupts () const noexcept { return rstFlag || nmiFlag || ( irqAssertedOnPin && ! flags.getI () ); }

	sidinline void buildInstructionTable () noexcept;

public:
	MOS6510 ( EventScheduler& scheduler, c64cpubus& bus );
	~MOS6510 () = default;

	/**
	* Get data from system environment.
	*
	* @param address
	* @return data byte CPU requested
	*/
	sidinline uint8_t cpuRead ( uint16_t addr ) noexcept	{	return dataBus.cpuRead ( addr );	}

	/**
	* Write data to system environment.
	*
	* @param address
	* @param data
	*/
	sidinline void cpuWrite ( uint16_t addr, uint8_t data ) noexcept	{	dataBus.cpuWrite ( addr, data );	}

	void reset () noexcept;

	static const char* credits ();

	void setRDY ( bool newRDY ) noexcept;

	// Non-standard functions
	void triggerRST () noexcept;
	void triggerNMI () noexcept;
	void triggerIRQ () noexcept;
	void clearIRQ () noexcept;
};

}
