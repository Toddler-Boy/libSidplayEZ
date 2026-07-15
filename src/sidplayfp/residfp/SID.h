#pragma once
/*
* This file is part of libSidplayEZ, a SID player engine based on libsidplayfp.
*
* Copyright 2024-2026 Michael Hartmann <mike@ultrasid.com>
* Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
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

/**
* The waveform D/A converter introduces a DC offset in the signal
* to the envelope multiplying D/A converter. The "zero" level of
* the waveform D/A converter can be found as follows:
*
* Measure the "zero" voltage of voice 3 on the SID audio output
* pin, routing only voice 3 to the mixer ($d417 = $0b, $d418 =
* $0f, all other registers zeroed).
*
* Then set the sustain level for voice 3 to maximum and search for
* the waveform output value yielding the same voltage as found
* above. This is done by trying out different waveform output
* values until the correct value is found, e.g. with the following
* program:
*
*        lda #$08
*        sta $d412
*        lda #$0b
*        sta $d417
*        lda #$0f
*        sta $d418
*        lda #$f0
*        sta $d414
*        lda #$21
*        sta $d412
*        lda #$01
*        sta $d40e
*
*        ldx #$00
*        lda #$38        ; Tweak this to find the "zero" level
*l       cmp $d41b
*        bne l
*        stx $d40e        ; Stop frequency counter - freeze waveform output
*        brk
*
* The waveform output range is 0x000 to 0xfff, so the "zero"
* level should ideally have been 0x800. In the measured chip, the
* waveform output "zero" level was found to be 0x380 (i.e. $d41b
* = 0x38) at an audio output voltage of 5.94V.
*
* With knowledge of the mixer op-amp characteristics, further estimates
* of waveform voltages can be obtained by sampling the EXT IN pin.
* From EXT IN samples, the corresponding waveform output can be found by
* using the model for the mixer.
*
* Such measurements have been done on a chip marked MOS 6581R4AR
* 0687 14, and the following results have been obtained:
* * The full range of one voice is approximately 1.5V.
* * The "zero" level rides at approximately 5.0V.
*
*
* zero-x did the measuring on the 8580 (https://sourceforge.net/p/vice-emu/bugs/1036/#c5b3):
* When it sits on basic from powerup it's at 4.72
* Run 1.prg and check the output pin level.
* Then run 2.prg and adjust it until the output level is the same...
* 0x94-0xA8 gives me the same 4.72 1.prg shows.
* On another 8580 it's 0x90-0x9C
* Third chip 0x94-0xA8
* Fourth chip 0x90-0xA4
* On the 8580 that plays digis the output is 4.66 and 0x93 is the only value to reach that.
* To me that seems as regular 8580s have somewhat wide 0-level range,
* whereas that digi-compatible 8580 has it very narrow.
* On my 6581R4AR has 0x3A as the only value giving the same output level as 1.prg
*/

/**
* Bus value stays alive for some time after each operation.
* Values differs between chip models, the timings used here
* are taken from VICE [1].
* See also the discussion "How do I reliably detect 6581/8580 sid?" on CSDb [2].
*
*   Results from real C64 (testprogs/SID/bitfade/delayfrq0.prg):
*
*   (new SID) (250469/8580R5) (250469/8580R5)
*   delayfrq0    ~7a000        ~108000
*
*   (old SID) (250407/6581)
*   delayfrq0    ~01d00
*
* [1]: http://sourceforge.net/p/vice-emu/patches/99/
* [2]: http://noname.c64.org/csdb/forums/?roomid=11&topicid=29025&showallposts=1
*/

#include <algorithm>
#include <limits>

namespace reSIDfp
{
	using ChipModel = enum : std::uint8_t
	{
		MOS6581 = 1,
		MOS8580
	};

	using CombinedWaveforms = enum : std::uint8_t
	{
		WEAK,
		AVERAGE,
		STRONG
	};
}

#include "Dac.h"

#include "Filter6581.h"
#include "Filter8580.h"

#include "WaveformCalculator.h"
#include "resample/TwoPassSincResampler.h"

#include "../../EZ/config.h"

#include "ExternalFilter.h"
#include "Voice.h"

namespace reSIDfp
{

//-----------------------------------------------------------------------------

/**
* MOS6581/MOS8580 emulation.
*/
template <typename FLT>
class SID final
{
private:
	static constexpr bool is6581 = std::is_same_v<FLT, Filter6581<true>> || std::is_same_v<FLT, Filter6581<false>>;

	FLT		filter;

	// External filter that provides high-pass and low-pass filtering to adjust sound tone slightly
	ExternalFilter	externalFilter;

	// Table of waveforms
	std::vector<int16_t>	waveTable;
	std::vector<int16_t>	pulldownTable;

	// Resampler used by audio generation code
	TwoPassSincResampler<is6581 ? 3 : 5>	resampler;

	static constexpr int	numVoices = 3;

	// SID voices
	Voice<is6581>	voice[ numVoices ];

	// Used to determine if the filter ever gets used during playback
	uint8_t	filterUsage;

	// Used to determine if the filter ever gets used during playback
	int		volumeIndex = 0;
	int8_t	lastVolume = 0;

	// Start-up declick. Tunes that return from init routinely poke the volume and
	// filter registers at the very beginning (init leaves volume 0, first play sets
	// 15, some toggle 0/15; the first note may route a voice into the SID filter).
	// Each such change steps the mixer DC offset - the volume nibble scales it, and
	// $d417 routing / $d418 mode bits swap the filter output (at its own DC) in and out
	// of the mix - stepping the external filter's input and ringing its slow (~1.6 Hz)
	// DC-blocker into an audible pop. While the declick is active (armed by the player
	// at the init->play boundary, for handshake tunes only) the external filter is
	// re-settled to the current operating point on the first sample and on every
	// subsequent $d417/$d418 write, so those steps produce no ring.
	//
	// Gating is by silence, not time: some tunes have long silent intros yet poke the
	// volume register during them (e.g. Chris Huelsbeck's "Shades" - 4 s before the
	// music, volume play inside the first second). "Silence ended" is detected as the
	// first note trigger - the gate bit set in a voice control register - which is
	// caught in write() (notes can only start from a register write, so there is no
	// need to poll anything in the clock path; and it never inspects envelope levels,
	// which under the leaky-envelope model never reach zero). Gate defaults to 0, so a
	// set bit unambiguously means a note is starting - no previous-state compare needed.
	// The declick stays active for a guaranteed minimum, then until that first trigger,
	// up to a safety cap. Real music's volume/filter dynamics then pass untouched.
	//
	// Digi safety (settle-all, latch off on a sustained stream): a settle re-seats the
	// DC-blocker, absorbing a volume/filter write's DC step. EVERY write is settled while
	// active - a start-up burst must be absorbed as a group, because settling only a subset
	// re-seats the blocker to one write's level and then rings *worse* on the writes left
	// through (skipping 15->0 after settling ->15 steps from the raised seat). A normal tune
	// pokes the volume a handful of times (this file writes 15,0,7 within the first frame)
	// then leaves it - those few writes are all settled and the pop is gone. A volume-register
	// digi (Mahoney-style, $d418 at audio rate) instead writes forever: once
	// STARTUP_DECLICK_DIGI_RUN consecutive *rapid* writes are seen it is taken as a digi and
	// the whole declick releases, so the stream passes through intact (only its first few ms
	// were settled). This makes the declick safe to arm for every tune, including the
	// non-returning (digi/BASIC) ones. See ExternalFilter::settle and armStartupDeclick().
	bool	startupDeclickActive = false;
	bool	startupDeclickPending = false;
	bool	startupDeclickNoteSeen = false;		// a gate bit was set while active
	int		startupDeclickMinCycles = 0;		// guaranteed-active countdown
	int		startupDeclickCapCycles = 0;		// safety-cap countdown
	int		startupDeclickWriteGate = 0;		// >0 => the previous vol/filter write was rapid (recent)
	int		startupDeclickRapidRun = 0;			// consecutive rapid vol/filter writes seen

	static constexpr int	STARTUP_DECLICK_MIN_CYCLES =	  100'000;	// ~100 ms guaranteed
	static constexpr int	STARTUP_DECLICK_MAX_CYCLES = 15'000'000;	// ~15 s hard cap

	// A vol/filter write within this many cycles of the previous one counts as "rapid".
	// ~8 ms sits between frame-rate writes (<= 60 Hz, gap >= 16400 cyc: single pokes and
	// fades, each settled on its own) and digi sample rates (>= ~1 kHz, gap <= ~1000 cyc).
	static constexpr int	STARTUP_DECLICK_RAPID_GAP_CYCLES = 8'000;

	// A run of this many consecutive rapid writes is taken as a sustained digi stream and
	// releases the declick. A normal tune's start-up pokes number a handful, well under this.
	static constexpr int	STARTUP_DECLICK_DIGI_RUN = 16;

	// Time to live for the last written value
	int	busValueTtl;

	// Current chip model's bus value TTL
	static constexpr auto	BUS_TTL_6581 = 0x01D00;
	static constexpr auto	BUS_TTL_8580 = 0xA2000;

	static constexpr int	modelTTL = is6581 ? BUS_TTL_6581 : BUS_TTL_8580;

	// Time until #voiceSync must be run.
	unsigned int nextVoiceSync;

	// Dac leakage
	double	dacLeakage = 0.01;

	// Voice DC drift
	double	voiceDCDrift = 1.0;

	// Last written value
	uint8_t	busValue;

	/**
	* Emulated nonlinearity of the envelope DAC
	*/
	float	envDAC[ 256 ];

	/**
	* Emulated nonlinearity of the oscillator DAC
	*/
	float	oscDAC[ 4096 ];

	/**
	* Calculate the number of cycles according to current parameters
	* that it takes to reach sync
	*
	* @param sync whether to do the actual voice synchronization
	*/
	sidinline void voiceSync ( const bool sync ) noexcept
	{
		// Synchronize the 3 waveform generators
		if ( sync ) [[ unlikely ]]
		{
			voice[ 0 ].waveformGenerator.synchronize ( voice[ 1 ].waveformGenerator, voice[ 2 ].waveformGenerator );
			voice[ 1 ].waveformGenerator.synchronize ( voice[ 2 ].waveformGenerator, voice[ 0 ].waveformGenerator );
			voice[ 2 ].waveformGenerator.synchronize ( voice[ 0 ].waveformGenerator, voice[ 1 ].waveformGenerator );
		}

		// Calculate the time to next voice sync
		nextVoiceSync = std::numeric_limits<int>::max ();

		auto updateSync = [ this ] ( const int i, const int j )
		{
			auto&		wave = voice[ i ].waveformGenerator;
			const auto	freq = wave.readFreq ();

			if ( wave.readTest () || freq == 0 || ! voice[ j ].waveformGenerator.readSync () ) [[ likely ]]
				return;

			const auto	accumulator = wave.readAccumulator ();
			const auto	thisVoiceSync = ( ( 0x7FFFFF - accumulator ) & 0xFFFFFF ) / freq + 1;

			if ( thisVoiceSync < nextVoiceSync )
				nextVoiceSync = thisVoiceSync;
		};

		updateSync ( 0, 1 );
		updateSync ( 1, 2 );
		updateSync ( 2, 0 );
	}

	void recalculateDACs () noexcept
	{
		constexpr auto	ENV_DAC_BITS = 8u;
		constexpr auto	OSC_DAC_BITS = 12u;

		constexpr auto	MOSFET_LEAKAGE_6581 = 0.0075;
		constexpr auto	MOSFET_LEAKAGE_8580 = 0.0035;

		const auto	dacLeakFactor = ( is6581 ? MOSFET_LEAKAGE_6581 : MOSFET_LEAKAGE_8580 ) * dacLeakage;

		// calculate envelope DAC table
		{
			Dac	dacBuilder ( ENV_DAC_BITS, dacLeakFactor );
			dacBuilder.kinkedDac ( is6581 );

			for ( auto i = 0u; i < ( 1 << ENV_DAC_BITS ); i++ )
				envDAC[ i ] = float ( dacBuilder.getOutput ( i ) );
		}

		// calculate oscillator DAC table
		{
			Dac	dacBuilder ( OSC_DAC_BITS, dacLeakFactor );
			dacBuilder.kinkedDac ( is6581 );

			const auto	offset = dacBuilder.getOutput ( 0x7ff, is6581 );// model == MOS6581 ? OFFSET_6581 : OFFSET_8580 );

			for ( auto i = 0u; i < ( 1 << OSC_DAC_BITS ); i++ )
				oscDAC[ i ] = float ( dacBuilder.getOutput ( i ) - offset );
		}
	}

	/**
	* Drive the external filter for one sample, honouring a pending start-up declick.
	*
	* When the first master-volume change of the tune is waiting to be handled, snap
	* the external filter to the current input DC instead of clocking it, so the volume
	* step produces no DC-blocker transient. All other samples clock normally.
	*/
	[[ nodiscard ]] sidinline int clockExternalFilter ( int extIn ) noexcept
	{
		if ( startupDeclickPending ) [[ unlikely ]]
		{
			startupDeclickPending = false;
			externalFilter.settle ( extIn );
			return 0;
		}

		return externalFilter.clock ( extIn );
	}

	/**
	* Route a voice control-register write, watching the gate bit for the declick.
	*
	* Notes can only start from a control-register write, so the start-up declick keys
	* its "silence ended" decision off the gate bit here rather than polling envelope
	* levels in the clock path (which is both hotter and, under the leaky-envelope model
	* where envelopes never reach zero, wrong). Gate defaults to 0, so any set bit means
	* a note is starting - no previous-state compare is needed.
	*/
	sidinline void writeControlReg ( int idx, uint8_t value ) noexcept
	{
		if ( startupDeclickActive && ( value & 0x01 ) ) [[ unlikely ]]
			startupDeclickNoteSeen = true;

		voice[ idx ].writeCONTROL_REG ( value );
	}

	/**
	* Handle a volume/filter ($d418/$d417) write for the start-up declick.
	*
	* While the declick is active, re-settle the external filter on EVERY write, so a
	* start-up burst of pokes is absorbed as a group (settling only a subset re-seats the
	* DC-blocker and rings worse - see the field comments). Meanwhile count consecutive
	* rapid writes: a long run is a volume-register digi, whose $d418 writes are the audio,
	* so the whole declick releases and the stream passes through un-settled.
	* See startupDeclickRapidRun.
	*/
	sidinline void declickVolFilterWrite () noexcept
	{
		if ( ! startupDeclickActive ) [[ likely ]]
			return;

		// Track the run of consecutive rapid writes; a sustained run means a digi -> release.
		if ( startupDeclickWriteGate > 0 )
		{
			if ( ++startupDeclickRapidRun >= STARTUP_DECLICK_DIGI_RUN ) [[ unlikely ]]
			{
				startupDeclickActive = false;		// confirmed digi: stop declicking entirely
				return;
			}
		}
		else
		{
			startupDeclickRapidRun = 0;
		}

		startupDeclickWriteGate = STARTUP_DECLICK_RAPID_GAP_CYCLES;
		startupDeclickPending = true;				// settle this write
	}

public:
	SID ()
	{
		waveTable = WaveformCalculator::buildWaveTable ();

		reset ();

		recalculateDACs ();

		// set voice tables
		for ( auto& vce : voice )
		{
			vce.setEnvDAC ( envDAC );
			vce.setWavDAC ( oscDAC );
			vce.waveformGenerator.setWaveformModels ( waveTable );
		}

		setCombinedWaveforms ( CombinedWaveforms::AVERAGE, 1.0f );
	}

	/**
	* Get currently emulated chip model.
	*/
	[[ nodiscard ]] ChipModel getChipModel () const noexcept { return is6581 ? ChipModel::MOS6581 : ChipModel::MOS8580; }

	/**
	* Set combined waveforms strength.
	*/
	void setCombinedWaveforms ( CombinedWaveforms cws, const float threshold ) noexcept
	{
		WaveformCalculator::buildPulldownTable ( pulldownTable, is6581, cws, threshold );

		for ( auto& vce : voice )
			vce.waveformGenerator.setPulldownModels ( pulldownTable );
	}

	/**
	* Set DAC leakage
	*/
	void setDacLeakage ( const double leakage ) noexcept
	{
		dacLeakage = leakage;
		recalculateDACs ();
	}

	/**
	* Set Voice DC drift
	*/
	void setVoiceDCDrift ( const double drift ) noexcept
	{
		voiceDCDrift = drift;

		if constexpr ( is6581 )
			filter.setVoiceDCDrift ( drift );
	}

	/**
	* Set Saw+Pulse-ultra-loud
	*/
	void setSawPulseUltra ( const bool enabled ) noexcept
	{
		if constexpr ( is6581 )
		{
			for ( auto& vce : voice )
				vce.waveformGenerator.setSawPulseMask ( enabled ? 0xffffff : 0x7fffff );
		}
	}

	/**
	* Set the 6581 charge-leakage rate.
	*
	* Speed multiplier for the modeled charge leakage: 1.0 = R4-class/warm (default),
	* ~10 = R3-class/warm, lower = colder chip. No-op for the 8580.
	*
	* @see WaveformGenerator::setLeakageRate(double)
	*/
	void setLeakageRate ( const double rate ) noexcept
	{
		if constexpr ( is6581 )
		{
			for ( auto& vce : voice )
				vce.waveformGenerator.setLeakageRate ( rate );
		}
	}

	/**
	* SID reset.
	*/
	void reset () noexcept
	{
		for ( auto& vce : voice )
			vce.reset ();

		filter.reset ();
		externalFilter.reset ();

		resampler.reset ();

		filterUsage = 0;
		volumeIndex = 0;
		lastVolume = 0;

		startupDeclickActive = false;
		startupDeclickPending = false;
		startupDeclickNoteSeen = false;
		startupDeclickMinCycles = 0;
		startupDeclickCapCycles = 0;
		startupDeclickWriteGate = 0;
		startupDeclickRapidRun = 0;

		busValue = 0;
		busValueTtl = 0;
		voiceSync ( false );
	}

	/**
	* Activate the start-up declick.
	*
	* Call once at the init -> playback boundary (after the tune's init has run,
	* before captured output begins). Safe for every tune - including non-returning
	* (digi/BASIC) ones - thanks to the write rate gate (see startupDeclickWriteGate).
	* Settles the external filter to the current operating point immediately (removing
	* any residual DC-blocker ring at capture start) and keeps re-settling it on each
	* *isolated* $d417/$d418 write until the first note trigger (a gate bit set in a voice
	* control register) - guaranteed for STARTUP_DECLICK_MIN_CYCLES, then until that trigger,
	* capped at STARTUP_DECLICK_MAX_CYCLES. Dense (digi-rate) write streams are not settled.
	* Deliberately separate from reset(): reset() issues a $d418 write that would
	* otherwise be treated as part of the opening dance.
	*/
	void armStartupDeclick () noexcept
	{
		startupDeclickActive = true;
		startupDeclickPending = true;
		startupDeclickNoteSeen = false;
		startupDeclickMinCycles = STARTUP_DECLICK_MIN_CYCLES;
		startupDeclickCapCycles = STARTUP_DECLICK_MAX_CYCLES;
		startupDeclickWriteGate = 0;			// first write starts a fresh run
		startupDeclickRapidRun = 0;
	}

	/**
	* Read registers
	*
	* Reading a write only register returns the last uint8_t written to any SID register.
	* The individual bits in this value start to fade down towards zero after a few cycles.
	* All bits reach zero within approximately $2000 - $4000 cycles.
	* It has been claimed that this fading happens in an orderly fashion,
	* however sampling of write only registers reveals that this is not the case.
	* NOTE: This is not correctly modeled.
	* The actual use of write only registers has largely been made
	* in the belief that all SID registers are readable.
	* To support this belief the read would have to be done immediately
	* after a write to the same register (remember that an intermediate write
	* to another register would yield that value instead).
	* With this in mind we return the last value written to any SID register
	* for $2000 cycles without modeling the bit fading.
	*
	* @param offset SID register to read
	* @return value read from chip
	*/
	[[ nodiscard ]] sidinline uint8_t read ( int offset ) noexcept
	{
		switch ( offset )
		{
			case 0x19: // X value of paddle
			case 0x1a: // Y value of paddle
				busValue = 0xff;
				busValueTtl = modelTTL;
				break;

			case 0x1b: // Voice #3 waveform output
				busValue = voice[ 2 ].waveformGenerator.readOSC ();
				busValueTtl = modelTTL;
				break;

			case 0x1c: // Voice #3 ADSR output
				busValue = voice[ 2 ].envelopeGenerator.readENV ();
				busValueTtl = modelTTL;
				break;

			default:
				// Reading from a write-only or non-existing register makes the bus discharge faster.
				// Emulate this by halving the residual TTL.
				busValueTtl /= 2;
				break;
		}

		return busValue;
	}

	/**
	* Write registers.
	*
	* @param offset chip register to write
	* @param value value to write
	*/
	sidinline void write ( int offset, uint8_t value ) noexcept
	{
		busValue = value;
		busValueTtl = modelTTL;

		switch ( offset )
		{
			case 0x00:	voice[ 0 ].waveformGenerator.writeFREQ_LO ( value );			break;	// Voice #1 frequency (Low-byte)
			case 0x01:	voice[ 0 ].waveformGenerator.writeFREQ_HI ( value );			break;	// Voice #1 frequency (High-byte)
			case 0x02:	voice[ 0 ].waveformGenerator.writePW_LO ( value );				break;	// Voice #1 pulse width (Low-byte)
			case 0x03:	voice[ 0 ].waveformGenerator.writePW_HI ( value );				break;	// Voice #1 pulse width (bits #8-#15)
			case 0x04:	writeControlReg ( 0, value );									break;	// Voice #1 control register
			case 0x05:	voice[ 0 ].envelopeGenerator.writeATTACK_DECAY ( value );		break;	// Voice #1 Attack and Decay length
			case 0x06:	voice[ 0 ].envelopeGenerator.writeSUSTAIN_RELEASE ( value );	break;	// Voice #1 Sustain volume and Release length
			case 0x07:	voice[ 1 ].waveformGenerator.writeFREQ_LO ( value );			break;	// Voice #2 frequency (Low-byte)
			case 0x08:	voice[ 1 ].waveformGenerator.writeFREQ_HI ( value );			break;	// Voice #2 frequency (High-byte)
			case 0x09:	voice[ 1 ].waveformGenerator.writePW_LO ( value );				break;	// Voice #2 pulse width (Low-byte)
			case 0x0a:	voice[ 1 ].waveformGenerator.writePW_HI ( value );				break;	// Voice #2 pulse width (bits #8-#15)
			case 0x0b:	writeControlReg ( 1, value );									break;	// Voice #2 control register
			case 0x0c:	voice[ 1 ].envelopeGenerator.writeATTACK_DECAY ( value );		break;	// Voice #2 Attack and Decay length
			case 0x0d:	voice[ 1 ].envelopeGenerator.writeSUSTAIN_RELEASE ( value );	break;	// Voice #2 Sustain volume and Release length
			case 0x0e:	voice[ 2 ].waveformGenerator.writeFREQ_LO ( value );			break;	// Voice #3 frequency (Low-byte)
			case 0x0f:	voice[ 2 ].waveformGenerator.writeFREQ_HI ( value );			break;	// Voice #3 frequency (High-byte)
			case 0x10:	voice[ 2 ].waveformGenerator.writePW_LO ( value );				break;	// Voice #3 pulse width (Low-byte)
			case 0x11:	voice[ 2 ].waveformGenerator.writePW_HI ( value );				break;	// Voice #3 pulse width (bits #8-#15)
			case 0x12:	writeControlReg ( 2, value );									break;	// Voice #3 control register
			case 0x13:	voice[ 2 ].envelopeGenerator.writeATTACK_DECAY ( value );		break;	// Voice #3 Attack and Decay length
			case 0x14:	voice[ 2 ].envelopeGenerator.writeSUSTAIN_RELEASE ( value );	break;	// Voice #3 Sustain volume and Release length
			case 0x15:	filter.writeFC_LO ( value ); 									break;	// Filter cut off frequency (bits #0-#2)
			case 0x16:	filter.writeFC_HI ( value ); 									break;	// Filter cut off frequency (bits #3-#10)
			case 0x17:																			// Filter control
			{
				filter.writeRES_FILT ( value );
				filterUsage |= value;

				// While the start-up declick is active, re-settle the external filter on an
				// isolated write: routing a voice into/out of the SID filter shifts the DC
				// path and would otherwise ring the DC-blocker into a pop. (Rate-gated so a
				// digi's dense $d418 stream is left intact - see declickVolFilterWrite.)
				declickVolFilterWrite ();
			}
			break;
			case 0x18:																			// Volume and filter modes
			{
				// While the start-up declick is active, re-settle the external filter on an
				// isolated volume/mode write. Both the volume nibble (scales the mixer DC)
				// and the filter-mode bits (route the filter output, at its own DC, into the
				// mixer) step the DC and would otherwise ring it. Rate-gated so a volume-
				// register digi's audio-rate writes pass through un-settled.
				declickVolFilterWrite ();

				filter.writeMODE_VOL ( value );
				lastVolume = int8_t ( value & 0x0F );
			}
			break;

			default:
				break;
		}

		// Update voicesync just in case
		voiceSync ( false );
	}

	/**
	* Setting of SID sampling parameters.
	*
	* Use a clock frequency of 985248Hz for PAL C64, 1022730Hz for NTSC C64.
	* The default end of passband frequency is pass_freq = 0.9*sample_freq/2
	* for sample frequencies up to ~ 44.1kHz, and 20kHz for higher sample frequencies.
	*
	* For resampling, the ratio between the clock frequency and the sample frequency
	* is limited as follows: 125*clock_freq/sample_freq < 16384
	* E.g. provided a clock frequency of ~ 1MHz, the sample frequency can not be set
	* lower than ~ 8kHz. A lower sample frequency would make the resampling code
	* overfill its 16k sample ring buffer.
	*
	* The end of passband frequency is also limited: pass_freq <= 0.9*sample_freq/2
	*
	* E.g. for a 44.1kHz sampling rate the end of passband frequency
	* is limited to slightly below 20kHz.
	* This constraint ensures that the FIR table is not overfilled.
	*
	* @param clockFrequency System clock frequency at Hz
	* @param samplingFrequency Desired output sampling rate
	* @param highestAccurateFrequency
	* @throw SIDError
	*/
	void setSamplingParameters ( double clockFrequency, double samplingFrequency ) noexcept
	{
		externalFilter.setClockFrequency ( clockFrequency );

		resampler.setup ( clockFrequency, samplingFrequency );
	}

	/**
	* Clock SID forward using chosen output sampling algorithm.
	*
	* @param cycles c64 clocks to clock
	* @param buf audio output buffer
	* @return number of samples produced
	*/
	sidinline int clock ( unsigned int cycles, int16_t* buf, int8_t* volRegBuf ) noexcept
	{
		// ageBusValue
		if ( busValueTtl ) [[ likely ]]
		{
			if ( busValueTtl -= cycles; busValueTtl <= 0 ) [[ unlikely ]]
			{
				busValue = 0;
				busValueTtl = 0;
			}
		}

		// Maintain the start-up declick window: active for a guaranteed minimum, then
		// until the first note trigger (startupDeclickNoteSeen, set from a gate bit in
		// write()), released also at the safety cap. Only the two countdowns live in the
		// clock path - no envelope polling, so the leaky-envelope model can't defeat it.
		if ( startupDeclickActive ) [[ unlikely ]]
		{
			startupDeclickMinCycles -= int ( cycles );
			startupDeclickCapCycles -= int ( cycles );

			// Age the rapid-write timer: while it is running the next volume/filter write
			// counts as "rapid" (continuing a fast run); once it expires the run resets.
			if ( startupDeclickWriteGate > 0 )
				startupDeclickWriteGate -= int ( cycles );

			if ( ( startupDeclickCapCycles <= 0 )
				|| ( ( startupDeclickMinCycles <= 0 ) && startupDeclickNoteSeen ) )
				startupDeclickActive = false;
		}

		auto output = [ this ] () -> int
		{
			const auto	o1 = voice[ 0 ].output ( voice[ 2 ].waveformGenerator );
			const auto	o2 = voice[ 1 ].output ( voice[ 0 ].waveformGenerator );
			const auto	o3 = voice[ 2 ].output ( voice[ 1 ].waveformGenerator );

			if constexpr ( is6581 )
			{
				const auto	env1 = voice[ 0 ].envelopeGenerator.output ();
				const auto	env2 = voice[ 1 ].envelopeGenerator.output ();
				const auto	env3 = voice[ 2 ].envelopeGenerator.output ();

				const auto	input = int ( filter.clock ( o1, o2, o3, env1, env2, env3 ) );
				return clockExternalFilter ( input + INT16_MIN );
			}
			else
			{
 				const auto	input = int ( filter.clock ( o1, o2, o3 ) );
 				return clockExternalFilter ( input + INT16_MIN );
 			}
		};

		auto    s = 0;
		while ( cycles ) [[ likely ]]
		{
			if ( const auto delta_t = std::min ( nextVoiceSync, cycles ); delta_t > 0 ) [[ likely ]]
			{
				for ( auto i = 0u; i < delta_t; i++ ) [[ likely ]]
				{
					// clock waveform generators
					voice[ 0 ].waveformGenerator.clock ();
					voice[ 1 ].waveformGenerator.clock ();
					voice[ 2 ].waveformGenerator.clock ();

					// clock envelope generators
					voice[ 0 ].envelopeGenerator.clock ();
					voice[ 1 ].envelopeGenerator.clock ();
					voice[ 2 ].envelopeGenerator.clock ();

					if ( resampler.input ( output () ) ) [[ unlikely ]]
					{
						buf[ s ] = resampler.output ();
						volRegBuf[ s ] = lastVolume;

						++s;
					}
				}

				cycles -= delta_t;
				nextVoiceSync -= delta_t;
			}

			if ( ! nextVoiceSync ) [[ unlikely ]]
				voiceSync ( true );
		}

		return s;
	}

	/**
	* Set filter curve parameter for 6581 model.
	*
	* @see Filter6581::setFilterCurve(double)
	*/
	void setFilter6581Curve ( [[ maybe_unused ]] double filterCurve ) noexcept
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setFilterCurve ( filterCurve );
	}

	/**
	* Set filter uCox and Cap for 6581 model
	*
	* @see Filter6581::setFilter_uCoxAndCap(double,bool)
	*/
	void setFilter6581_uCoxAndCap ( [[ maybe_unused ]] double uCox, [[ maybe_unused ]] bool oldCap ) noexcept
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setFilter_uCoxAndCap ( uCox, oldCap );
	}

	/**
	* Set filter gain parameter for 6581 model
	*
	* @see Filter6581::setFilterGain(double)
	*/
	void setFilter6581Gain ( [[ maybe_unused ]] double adjustment ) noexcept
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setFilterGain ( adjustment );
	}

	/**
	* Set filter saturation/distortion drive for 6581 model.
	*
	* @see Filter6581::setFilterSaturation(double)
	*/
	void setFilter6581Saturation ( [[ maybe_unused ]] double saturation ) noexcept
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setFilterSaturation ( saturation );
	}

	/**
	* Set filter bandpass width offset for 6581 model.
	*
	* @see Filter6581::setBandpassWidthOffset(double)
	*/
	void setFilter6581BandpassWidthOffset ( [[ maybe_unused ]] double offset ) noexcept
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setBandpassWidthOffset ( offset );
	}

	/**
	* Set filter digi volume for 6581 model
	*
	* @see Filter6581::setDigitVolume(double)
	*/
	void setFilter6581Digi ( [[ maybe_unused ]] double adjustment ) noexcept
	{
		if constexpr ( is6581 )
			filter.setDigiVolume ( adjustment );
	}

	/**
	* Set filter curve parameter for 8580 model.
	*
	* @see Filter8580::setFilterCurve(double)
	*/
	void setFilter8580Curve ( [[ maybe_unused ]] double filterCurve ) noexcept
	{
		if constexpr ( std::is_same_v<FLT, Filter8580<true>> )
			filter.setFilterCurve ( filterCurve );
	}

	[[ nodiscard ]] float getEnvLevel ( int voiceNo ) const noexcept {	return voice[ voiceNo ].getEnvLevel (); }
	[[ nodiscard ]] bool wasFilterUsed () const noexcept {	return filterUsage & 0x0F;		}
};
//-----------------------------------------------------------------------------

} // namespace reSIDfp
