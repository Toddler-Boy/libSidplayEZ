#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2023 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2004,2010 Dag Lem <resid@nimrod.no>
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

#include <algorithm>
#include <vector>

#include "../../EZ/config.h"

namespace reSIDfp
{

/*
* This is what happens when the lfsr is clocked:
*
* cycle 0: bit 19 of the accumulator goes from low to high, the noise register acts normally,
*          the output may pulldown a bit;
*
* cycle 1: first phase of the shift, the bits are interconnected and the output of each bit
*          is latched into the following. The output may overwrite the latched value.
*
* cycle 2: second phase of the shift, the latched value becomes active in the first
*          half of the clock and from the second half the register returns to normal operation.
*
* When the test or reset lines are active the first phase is executed at every cyle
* until the signal is released triggering the second phase.
*
*      |       |    bit n     |   bit n+1
*      | bit19 | latch output | latch output
* -----+-------+--------------+--------------
* phi1 |   0   |   A <-> A    |   B <-> B
* phi2 |   0   |   A <-> A    |   B <-> B
* -----+-------+--------------+--------------
* phi1 |   1   |   A <-> A    |   B <-> B      <- bit19 raises
* phi2 |   1   |   A <-> A    |   B <-> B
* -----+-------+--------------+--------------
* phi1 |   1   |   X     A  --|-> A     B      <- shift phase 1
* phi2 |   1   |   X     A  --|-> A     B
* -----+-------+--------------+--------------
* phi1 |   1   |   X --> X    |   A --> A      <- shift phase 2
* phi2 |   1   |   X <-> X    |   A <-> A
*
*
* Normal cycles
* -------------
* Normally, when noise is selected along with another waveform,
* c1 and c2 are closed and the output bits pull down the corresponding
* shift register bits.
*
*        noi_out_x             noi_out_x+1
*          ^                     ^
*          |                     |
*          +-------------+       +-------------+
*          |             |       |             |
*          +---o<|---+   |       +---o<|---+   |
*          |         |   |       |         |   |
*       c2 |      c1 |   |    c2 |      c1 |   |
*          |         |   |       |         |   |
*  >---/---+---|>o---+   +---/---+---|>o---+   +---/--->
*      LC                    LC                    LC
*
*
* Shift phase 1
* -------------
* During shift phase 1 c1 and c2 are open, the SR bits are floating
* and will be driven by the output of combined waveforms,
* or slowly turn high.
*
*        noi_out_x             noi_out_x+1
*          ^                     ^
*          |                     |
*          +-------------+       +-------------+
*          |             |       |             |
*          +---o<|---+   |       +---o<|---+   |
*          |         |   |       |         |   |
*       c2 /      c1 /   |    c2 /      c1 /   |
*          |         |   |       |         |   |
*  >-------+---|>o---+   +-------+---|>o---+   +------->
*      LC                    LC                    LC
*
*
* Shift phase 2 (phi1)
* --------------------
* During the first half cycle of shift phase 2 c1 is closed
* so the value from of noi_out_x-1 enters the bit.
*
*        noi_out_x             noi_out_x+1
*          ^                     ^
*          |                     |
*          +-------------+       +-------------+
*          |             |       |             |
*          +---o<|---+   |       +---o<|---+   |
*          |         |   |       |         |   |
*       c2 /      c1 |   |    c2 /      c1 |   |
*          |         |   |       |         |   |
*  >---/---+---|>o---+   +---/---+---|>o---+   +---/--->
*      LC                    LC                    LC
*
*
* Shift phase 2 (phi2)
* --------------------
* On the second half of shift phase 2 c2 closes and
* we're back to normal cycles.
*/

sidinline unsigned int get_noise_writeback ( unsigned int waveform_output ) noexcept
{
	return
		( ( waveform_output & ( 1u << 11 ) ) >> 9 ) |  // Bit 11 -> bit 20
		( ( waveform_output & ( 1u << 10 ) ) >> 6 ) |  // Bit 10 -> bit 18
		( ( waveform_output & ( 1u << 9 ) ) >> 1 ) |  // Bit  9 -> bit 14
		( ( waveform_output & ( 1u << 8 ) ) << 3 ) |  // Bit  8 -> bit 11
		( ( waveform_output & ( 1u << 7 ) ) << 6 ) |  // Bit  7 -> bit  9
		( ( waveform_output & ( 1u << 6 ) ) << 11 ) |  // Bit  6 -> bit  5
		( ( waveform_output & ( 1u << 5 ) ) << 15 ) |  // Bit  5 -> bit  2
		( ( waveform_output & ( 1u << 4 ) ) << 18 );   // Bit  4 -> bit  0
}

/**
* A 24 bit accumulator is the basis for waveform generation.
* FREQ is added to the lower 16 bits of the accumulator each cycle.
* The accumulator is set to zero when TEST is set, and starts counting
* when TEST is cleared.
*
* Waveforms are generated as follows:
*
* - No waveform:
* When no waveform is selected, the DAC input is floating.
*
*
* - Triangle:
* The upper 12 bits of the accumulator are used.
* The MSB is used to create the falling edge of the triangle by inverting
* the lower 11 bits. The MSB is thrown away and the lower 11 bits are
* left-shifted (half the resolution, full amplitude).
* Ring modulation substitutes the MSB with MSB EOR NOT sync_source MSB.
*
*
* - Sawtooth:
* The output is identical to the upper 12 bits of the accumulator.
*
*
* - Pulse:
* The upper 12 bits of the accumulator are used.
* These bits are compared to the pulse width register by a 12 bit digital
* comparator; output is either all one or all zero bits.
* The pulse setting is delayed one cycle after the compare.
* The test bit, when set to one, holds the pulse waveform output at 0xfff
* regardless of the pulse width setting.
*
*
* - Noise:
* The noise output is taken from intermediate bits of a 23-bit shift register
* which is clocked by bit 19 of the accumulator.
* The shift is delayed 2 cycles after bit 19 is set high.
*
* Operation: Calculate EOR result, shift register, set bit 0 = result.
*
*                    reset  +--------------------------------------------+
*                      |    |                                            |
*               test--OR-->EOR<--+                                       |
*                      |         |                                       |
*                      2 2 2 1 1 1 1 1 1 1 1 1 1                         |
*     Register bits:   2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 <---+
*                          |   |       |     |   |       |     |   |
*     Waveform bits:       1   1       9     8   7       6     5   4
*                          1   0
*
* The low 4 waveform bits are zero (grounded).
*/

// Waveform generator templated for 6581 or 8580
template <bool is6581>
class WaveformGenerator final
{
private:
	std::vector<int16_t>*	model_wave = nullptr;
	std::vector<int16_t>*	model_pulldown = nullptr;

	int16_t*	wave = nullptr;
	int16_t*	pulldown = nullptr;

	// PWout = (PWn/40.95)%
	unsigned int pw = 0;

	unsigned int shift_register = 0;

	/// Shift register is latched when transitioning to shift phase 1.
	unsigned int shift_latch;

	/// Emulation of pipeline causing bit 19 to clock the shift register.
	int shift_pipeline = 0;

	unsigned int ring_msb_mask = 0;
	unsigned int no_noise = 0;
	unsigned int noise_output = 0;
	unsigned int no_noise_or_noise_output = 0;
	unsigned int no_pulse = 0;
	unsigned int pulse_output = 0;

	/// The control register right-shifted 4 bits; used for output function table lookup.
	unsigned int waveform = 0;

	unsigned int waveform_output = 0;

	/// Current accumulator value.
	unsigned int accumulator = 0x555555;		// Accumulator's even bits are high on powerup
	unsigned int accumulatorMask = 0x7fffff;	// Mask to be used for 6581 with saw+pulse

	// Fout = (Fn*Fclk/16777216)Hz
	unsigned int freq = 0;

	/// 8580 tri/saw pipeline
	unsigned int tri_saw_pipeline = 0x555;

	/// The OSC3 value
	unsigned int osc3 = 0;

	/// Remaining time to fully reset shift register.
	unsigned int shift_register_reset = 0;

	// The wave signal TTL when no waveform is selected.
	unsigned int floating_output_ttl = 0;

	/*
	* 6581 charge-leakage model (see the comment block in writeCONTROL_REG() for the full story).
	*
	* leakageRate is a speed multiplier applied to the four charge-leakage timings modeled in this
	* class: effective_time = reference_time / leakageRate. It only affects the 6581; the 8580 keeps
	* its own fixed constants. Recomputed into the plain-integer members below by setLeakageRate() so
	* the per-cycle hot paths never divide.
	*/
	double leakageRate = 1.0;

	// Reference leakage timings at leakageRate == 1.0 (R4-class / warm chip), in cycles.
	static constexpr double	LEAKAGE_TTL_6581     = 1000000.0;	// ~1s    hold before floating DAC output decays
	static constexpr double	LEAKAGE_RESET_6581   = 2150000.0;	// ~2.15s test-bit hold before shift register resets
	static constexpr double	LEAKAGE_FADE_6581    = 38000.0;		// re-arm interval between floating DAC decay steps
	static constexpr double	LEAKAGE_SR_FADE_6581 = 241000.0;	// re-arm interval between shift-register bit-fade steps
																//   (8580 314300 * R4/8580 SR ratio 2.15/2.8 ~= 0.77)

	// Effective (rate-scaled) leakage timings. 6581 only; recomputed by setLeakageRate().
	unsigned int leakage_ttl     = (unsigned int)LEAKAGE_TTL_6581;
	unsigned int leakage_reset   = (unsigned int)LEAKAGE_RESET_6581;
	unsigned int leakage_fade    = (unsigned int)LEAKAGE_FADE_6581;
	unsigned int leakage_sr_fade = (unsigned int)LEAKAGE_SR_FADE_6581;

	/// The control register bits. Gate is handled by EnvelopeGenerator.
	//@{
	bool test = false;
	bool sync = false;
	//@}

	/// Test bit is latched at phi2 for the noise XOR.
	bool test_or_reset;

	/// Tell whether the accumulator MSB was set high on this cycle.
	bool msb_rising = false;

	static constexpr unsigned int	shift_mask =
		~(
			( 1u << 2 ) |  // Bit 20
			( 1u << 4 ) |  // Bit 18
			( 1u << 8 ) |  // Bit 14
			( 1u << 11 ) |  // Bit 11
			( 1u << 13 ) |  // Bit  9
			( 1u << 17 ) |  // Bit  5
			( 1u << 20 ) |  // Bit  2
			( 1u << 22 )    // Bit  0
		);

	/*
	* Perform the actual shifting, moving the latched value into following bits.
	* The XORing for bit0 is done in this cycle using the test bit latched during
	* the previous phi2 cycle.
	*/
	sidinline void shift_phase2 ( const unsigned int waveform_old, const unsigned int waveform_new ) noexcept
	{
		auto do_writeback = [ = ] () -> bool
		{
			// no writeback without combined waveforms (fixes SID/noisewriteback/noise_writeback_test2-{old,new})
			if ( waveform_old <= 8 )
				return false;

			if ( waveform_new < 8 )
				return false;

			if ( ( waveform_new == 8 )
				 // breaks noise_writeback_check_F_to_8_old
				 // but fixes simple and scan
				 && ( waveform_old != 0xf ) )
			{
				// fixes
				// noise_writeback_check_9_to_8_old
				// noise_writeback_check_A_to_8_old
				// noise_writeback_check_B_to_8_old
				// noise_writeback_check_D_to_8_old
				// noise_writeback_check_E_to_8_old
				// noise_writeback_check_F_to_8_old
				// noise_writeback_check_9_to_8_new
				// noise_writeback_check_A_to_8_new
				// noise_writeback_check_D_to_8_new
				// noise_writeback_check_E_to_8_new
				// noise_writeback_test1-{old,new}
				return false;
			}

			// What's happening here?
			if constexpr ( is6581 )
			{
				if (	( ( ( waveform_old & 0x3 ) == 0x1 ) && ( ( waveform_new & 0x3 ) == 0x2 ) )
					 || ( ( ( waveform_old & 0x3 ) == 0x2 ) && ( ( waveform_new & 0x3 ) == 0x1 ) ) )
				{
					// fixes
					// noise_writeback_check_9_to_A_old
					// noise_writeback_check_9_to_E_old
					// noise_writeback_check_A_to_9_old
					// noise_writeback_check_A_to_D_old
					// noise_writeback_check_D_to_A_old
					// noise_writeback_check_E_to_9_old
					return false;
				}
			}

			if ( waveform_old == 0xc )
			{
				// fixes
				// noise_writeback_check_C_to_A_new
				return false;
			}

			if ( waveform_new == 0xc )
			{
				// fixes
				// noise_writeback_check_9_to_C_old
				// noise_writeback_check_A_to_C_old
				return false;
			}

			// ok do the writeback
			return true;
		};

		// if noise is combined with another waveform the output drives the SR bits
		if ( do_writeback () )
			shift_latch = ( shift_register & shift_mask ) | get_noise_writeback ( waveform_output );

		// bit0 = (bit22 | test | reset) ^ bit17 = 1 ^ bit17 = ~bit17
		const unsigned int bit22 = ( ( test_or_reset ? 1 : 0 ) | shift_latch ) << 22;
		const unsigned int bit0 = ( bit22 ^ ( shift_latch << 17 ) ) & ( 1 << 22 );

		shift_register = ( shift_latch >> 1 ) | bit0;
		set_noise_output ();
	}
	//-----------------------------------------------------------------------------

	sidinline void write_shift_register () noexcept
	{
		if ( waveform > 0x8 ) [[ unlikely ]]
		{
			#if 0
			// FIXME this breaks SID/wf12nsr/wf12nsr
			if ( waveform == 0xc )
				// fixes
				// noise_writeback_check_8_to_C_old
				// noise_writeback_check_9_to_C_old
				// noise_writeback_check_A_to_C_old
				// noise_writeback_check_C_to_C_old
				return;
			#endif

			// Write changes to the shift register output caused by combined waveforms
			// back into the shift register.
			if ( shift_pipeline != 1 && ! test ) [[ likely ]]
			{
				// the output pulls down the SR bits
				shift_register = shift_register & ( shift_mask | get_noise_writeback ( waveform_output ) );
				noise_output &= waveform_output;
			}
			else
			{
				// shift phase 1: the output drives the SR bits
				noise_output = waveform_output;
			}

			set_no_noise_or_noise_output ();
		}
	}

	sidinline void set_noise_output () noexcept
	{
		noise_output =
			( ( shift_register & ( 1u << 2 ) ) << 9 ) |  // Bit 20 -> bit 11
			( ( shift_register & ( 1u << 4 ) ) << 6 ) |  // Bit 18 -> bit 10
			( ( shift_register & ( 1u << 8 ) ) << 1 ) |  // Bit 14 -> bit  9
			( ( shift_register & ( 1u << 11 ) ) >> 3 ) |  // Bit 11 -> bit  8
			( ( shift_register & ( 1u << 13 ) ) >> 6 ) |  // Bit  9 -> bit  7
			( ( shift_register & ( 1u << 17 ) ) >> 11 ) |  // Bit  5 -> bit  6
			( ( shift_register & ( 1u << 20 ) ) >> 15 ) |  // Bit  2 -> bit  5
			( ( shift_register & ( 1u << 22 ) ) >> 18 );   // Bit  0 -> bit  4

		set_no_noise_or_noise_output ();
	}

	sidinline void set_no_noise_or_noise_output () noexcept
	{
		no_noise_or_noise_output = no_noise | noise_output;
	}

	sidinline void shiftregBitfade () noexcept
	{
		shift_register |= shift_register >> 1;
		shift_register |= 0x400000;

		// Re-arm interval between shift-register bit-fade steps. Same physical charge-leakage effect
		// as the other three leakage timings; see setLeakageRate(). 6581: leakage_sr_fade is the
		// rate-scaled reference (~241000 at leakageRate 1.0, ~15000-class near rate 10).
		constexpr auto	SHIFT_REGISTER_FADE_8580R5 = 314300u;	// ~2.8s

		if ( shift_register != 0x7fffff )
			shift_register_reset = is6581 ? leakage_sr_fade : SHIFT_REGISTER_FADE_8580R5;
	}

public:
	void setWaveformModels ( std::vector<int16_t>& models )	noexcept	{	model_wave = &models;		}
	void setPulldownModels ( std::vector<int16_t>& models )	noexcept	{	model_pulldown = &models;	}
	void setSawPulseMask ( unsigned int mask ) noexcept					{	accumulatorMask = mask;		}

	/**
	* Set the 6581 charge-leakage rate.
	*
	* Speed multiplier for the four charge-leakage timings modeled in this class (floating DAC hold
	* TTL, floating DAC fade re-arm, shift-register reset, and shift-register bit-fade re-arm).
	* Effective time = reference_time / rate.
	*   rate == 1.0  -> R4-class / warm chip (the default; leaks ~10x slower than the old R3 model)
	*   rate ~= 10.0 -> R3-class / warm chip (reproduces the old fixed 6581R3 behavior within tolerance)
	*   rate  < 1.0  -> colder chip (leaks even slower)
	* No-op for the 8580, which keeps its own fixed constants. Safe to change at runtime without reset.
	*/
	void setLeakageRate ( double rate ) noexcept
	{
		if constexpr ( is6581 )
		{
			// Clamp to a sane range.
			rate = std::clamp ( rate, 0.1, 20.0 );

			leakageRate = rate;

			// Recompute the effective timings once here so the per-cycle hot paths only ever
			// touch plain integer members (no division). Worst case (rate 0.1) is ~21.5M cycles,
			// which fits comfortably in an unsigned int.
			leakage_ttl     = (unsigned int)( LEAKAGE_TTL_6581     / rate );
			leakage_reset   = (unsigned int)( LEAKAGE_RESET_6581   / rate );
			leakage_fade    = (unsigned int)( LEAKAGE_FADE_6581    / rate );
			leakage_sr_fade = (unsigned int)( LEAKAGE_SR_FADE_6581 / rate );
		}
	}

	/**
	* SID clocking.
	*/
	sidinline void clock () noexcept
	{
		if ( test ) [[ unlikely ]]
		{
			if ( shift_register_reset && ( --shift_register_reset == 0 ) ) [[ unlikely ]]
			{
				shiftregBitfade ();
				shift_latch = shift_register;

				// New noise waveform output.
				set_noise_output ();
			}

			// Latch the test bit value for shift phase 2.
			test_or_reset = true;

			// The test bit sets pulse high.
			pulse_output = 0xfff;
		}
		else
		{
			// Calculate new accumulator value;
			const auto	accumulator_old = accumulator;
			accumulator = ( accumulator + freq ) & 0xffffff;

			// Check which bit have changed from low to high
			const auto	accumulator_bits_set = ~accumulator_old & accumulator;

			// Check whether the MSB is set high. This is used for synchronization.
			msb_rising = accumulator_bits_set & 0x800000;

			// Shift noise register once for each time accumulator bit 19 is set high.
			// The shift is delayed 2 cycles.
			if ( accumulator_bits_set & 0x080000 ) [[ unlikely ]]
			{
				// Pipeline: Detect rising bit, shift phase 1, shift phase 2.
				shift_pipeline = 2;
			}
			else if ( shift_pipeline != 0 )
			{
				switch ( --shift_pipeline )
				{
					case 0:
						shift_phase2 ( waveform, waveform );
						break;

					case 1:
						// Start shift phase 1
						test_or_reset = false;
						shift_latch = shift_register;
						break;
				}
			}
		}
	}

	/**
	* Synchronize oscillators.
	* This must be done after all the oscillators have been clock()'ed,
	* so that they are in the same state.
	*
	* @param syncDest The oscillator that will be synced
	* @param syncSource The sync source oscillator
	*/
	sidinline void synchronize ( WaveformGenerator& syncDest, WaveformGenerator& syncSource ) const noexcept
	{
		// A special case occurs when a sync source is synced itself on the same
		// cycle as when its MSB is set high. In this case the destination will
		// not be synced. This has been verified by sampling OSC3.
		if ( msb_rising && syncDest.sync && ! ( sync && syncSource.msb_rising ) ) [[ unlikely ]]
			syncDest.accumulator = 0;
	}

	/**
	* Write FREQ LO register.
	*
	* @param freq_lo low 8 bits of frequency
	*/
	void writeFREQ_LO ( uint8_t freq_lo ) noexcept { freq = ( freq & 0xff00 ) | ( freq_lo & 0xff ); }

	/**
	* Write FREQ HI register.
	*
	* @param freq_hi high 8 bits of frequency
	*/
	void writeFREQ_HI ( uint8_t freq_hi ) noexcept { freq = ( freq_hi << 8 & 0xff00 ) | ( freq & 0xff ); }

	/**
	* Write PW LO register.
	*
	* @param pw_lo low 8 bits of pulse width
	*/
	void writePW_LO ( uint8_t pw_lo ) noexcept { pw = ( pw & 0xf00 ) | ( pw_lo & 0x0ff ); }

	/**
	* Write PW HI register.
	*
	* @param pw_hi high 8 bits of pulse width
	*/
	void writePW_HI ( uint8_t pw_hi ) noexcept { pw = ( pw_hi << 8 & 0xf00 ) | ( pw & 0x0ff ); }

	/**
	* Write CONTROL REGISTER register.
	*
	* @param control control register value
	*/
	void writeCONTROL_REG ( uint8_t control ) noexcept
	{
		const auto	waveform_prev = waveform;
		const auto	test_prev = test;

		waveform = ( control >> 4 ) & 0x0f;
		test = ( control & 0x08 ) != 0;
		sync = ( control & 0x02 ) != 0;

		// Substitution of accumulator MSB when sawtooth = 0, ring_mod = 1.
		ring_msb_mask = ( ( ~control >> 5 ) & ( control >> 2 ) & 0x1 ) << 23;

		if ( waveform != waveform_prev ) [[ unlikely ]]
		{
			auto	modWave = model_wave->data ();
			auto	modPulldown = model_pulldown->data ();

			// Set up waveform tables
			wave = &modWave[ ( waveform & 0x3 ) << 12 ];

			// We assume the combinations including noise behave the same as without
			switch ( waveform & 0x7 )
			{
				case 3:     pulldown = &modPulldown[ 0 << 12 ];									break;
				case 4:     pulldown = ( waveform & 0x8 ) ? &modPulldown[ 4 << 12 ] : nullptr;	break;
				case 5:     pulldown = &modPulldown[ 1 << 12 ];									break;
				case 6:     pulldown = &modPulldown[ 2 << 12 ];									break;
				case 7:     pulldown = &modPulldown[ 3 << 12 ];									break;
				default:    pulldown = nullptr;													break;
			}

			// no_noise and no_pulse are used in set_waveform_output() as bitmasks to
			// only let the noise or pulse influence the output when the noise or pulse
			// waveforms are selected.
			no_noise = ( waveform & 0x8 ) != 0 ? 0x000 : 0xfff;
			set_no_noise_or_noise_output ();
			no_pulse = ( waveform & 0x4 ) != 0 ? 0x000 : 0xfff;

			/**
			* Number of cycles after which the waveform output fades to 0 when setting the waveform register to 0.
			*
			* This is one of four timings that all model the same physical effect: charge slowly leaking away
			* on the 6581. The other three are the shift-register reset time (test-bit handling further down in
			* this function), the floating-DAC fade re-arm interval (in output()), and the shift-register
			* bit-fade re-arm interval (in shiftregBitfade()). All four were originally hardcoded to values
			* measured on warm 6581R3 chips.
			*
			* As the existing maintainer note put it, these times "vary wildly with temperature and may differ
			* from chip to chip". 6581R4-class chips in particular leak roughly 10x slower than R3 (close to the
			* 8580 values). To capture that spread the four 6581 timings are now driven by a single runtime
			* multiplier (see setLeakageRate()) instead of fixed constants: the reference values are the slow
			* R4/warm-chip timings at leakageRate == 1.0 (the new default), while a higher rate (~10) reproduces
			* the old fast R3 behavior and a lower rate models an even colder/slower chip. The 8580 is unaffected
			* and keeps its own fixed constants.
			*
			* Values measured on warm chips (6581R3/R4 and 8580R5) checking OSC3.
			*
			* See [VICE Bug #290](http://sourceforge.net/p/vice-emu/bugs/290/)
			* and [VICE Bug #1128](http://sourceforge.net/p/vice-emu/bugs/1128/)
			*/
			constexpr auto	FLOATING_OUTPUT_TTL_8580R5 = 800000u;	// ~1s

			// Change to floating DAC input.
			// Reset fading time for floating DAC input.
			// 6581: leakage_ttl is the rate-scaled reference (~1s at leakageRate 1.0, ~95ms near rate 10).
			if ( waveform == 0 )
				floating_output_ttl = is6581 ? leakage_ttl : FLOATING_OUTPUT_TTL_8580R5;
		}

		if ( test != test_prev ) [[ unlikely ]]
		{
			if ( test ) [[ unlikely ]]
			{
				// Reset accumulator.
				accumulator = 0;

				// Flush shift pipeline.
				shift_pipeline = 0;

				// Latch the shift register value.
				shift_latch = shift_register;

				/**
				* Number of cycles after which the shift register is reset when the test bit is set.
				*
				* Same physical charge-leakage effect as the floating-DAC TTL above; see that comment and
				* setLeakageRate(). The 6581 reference (leakage_reset, ~2.15s at leakageRate 1.0) is the slow
				* R4-class value; ~10x rate reproduces the old ~210ms R3 timing.
				* Values measured on warm chips (6581R3/R4 and 8580R5) checking OSC3.
				*/
				constexpr auto	SHIFT_REGISTER_RESET_8580R5 = 986000u;	// ~2.8s

				// Set reset time for shift register.
				shift_register_reset = is6581 ? leakage_reset : SHIFT_REGISTER_RESET_8580R5;
			}
			else
			{
				// When the test bit is falling, the second phase of the shift is
				// completed by enabling SRAM write.
				shift_phase2 ( waveform_prev, waveform );
			}
		}
	}

	/**
	* SID reset.
	*/
	void reset () noexcept
	{
		// accumulator is not changed on reset
		freq = 0;
		pw = 0;

		msb_rising = false;

		waveform = 0;
		osc3 = 0;

		test = false;
		sync = false;

		wave = nullptr;
		pulldown = nullptr;

		ring_msb_mask = 0;
		no_noise = 0xfff;
		no_pulse = 0xfff;
		pulse_output = 0xfff;

		shift_register_reset = 0;
		shift_register = 0x7fffff;
		// when reset is released the shift register is clocked once
		// so the lower bit is zeroed out
		// bit0 = (bit22 | test) ^ bit17 = 1 ^ 1 = 0
		test_or_reset = true;
		shift_latch = shift_register;
		shift_phase2 ( 0, 0 );

		shift_pipeline = 0;

		waveform_output = 0;
		floating_output_ttl = 0;
	}

	/**
	* 12-bit waveform output.
	*
	* @param ringModulator The oscillator ring-modulating current one.
	* @return the waveform generator digital output
	*/
	[[ nodiscard ]] sidinline unsigned int output ( const WaveformGenerator& ringModulator ) noexcept
	{
		// Set output value.
		if ( waveform ) [[ likely ]]
		{
			const auto	ix = ( accumulator ^ ( ~ringModulator.accumulator & ring_msb_mask ) ) >> 12;

			// The bit masks no_pulse and no_noise are used to achieve branch-free
			// calculation of the output value.
			waveform_output = wave[ ix ] & ( no_pulse | pulse_output ) & no_noise_or_noise_output;
			if ( pulldown )
				waveform_output = pulldown[ waveform_output ];

			if constexpr ( is6581 )
			{
				osc3 = waveform_output;

				// In the 6581 the top bit of the accumulator may be driven low by combined waveforms
				// when the sawtooth is selected
				if ( ( waveform & 0x2 ) && ( ( waveform_output & 0x800 ) == 0 ) )
				{
					msb_rising = false;
					if ( waveform == 0x6 ) [[ unlikely ]]
						accumulator &= accumulatorMask;
					else
						accumulator &= 0x7fffff;
				}
			}
			else
			{
				// Triangle/Sawtooth output is delayed half cycle on 8580
				// This will appear as a one cycle delay on OSC3 as it is latched in the first phase of the clock
				if ( waveform & 3 )
				{
					osc3 = tri_saw_pipeline & ( no_pulse | pulse_output ) & no_noise_or_noise_output;
					if ( pulldown )
						osc3 = pulldown[ osc3 ];

					tri_saw_pipeline = wave[ ix ];
				}
				else
				{
					osc3 = waveform_output;
				}
			}

			write_shift_register ();
		}
		else
		{
			// Age floating DAC input.
			if ( floating_output_ttl && ( --floating_output_ttl == 0 ) ) [[ unlikely ]]
			{
				// Re-arm interval between decay steps. Same charge-leakage effect as the floating-DAC
				// hold TTL and shift-register reset; see setLeakageRate(). 6581: leakage_fade is the
				// rate-scaled reference (~38000 at leakageRate 1.0, ~1400-class near rate 10).
				constexpr auto	FLOATING_OUTPUT_FADE_8580R5 = 50000u;

				waveform_output &= waveform_output >> 1;
				osc3 = waveform_output;
				if ( waveform_output )
					floating_output_ttl = is6581 ? leakage_fade : FLOATING_OUTPUT_FADE_8580R5;
			}
		}

		// The pulse level is defined as (accumulator >> 12) >= pw ? 0xfff : 0x000.
		// The expression -((accumulator >> 12) >= pw) & 0xfff yields the same
		// results without any branching (and thus without any pipeline stalls).
		// NB! This expression relies on that the result of a boolean expression
		// is either 0 or 1, and furthermore requires two's complement integer.
		// A few more cycles may be saved by storing the pulse width left shifted
		// 12 bits, and dropping the and with 0xfff (this is valid since pulse is
		// used as a bit mask on 12 bit values), yielding the expression
		// -(accumulator >= pw24). However this only results in negligible savings.

		// The result of the pulse width compare is delayed one cycle.
		// Push next pulse level into pulse level pipeline.
//		pulse_output = ( ( accumulator >> 12 ) >= pw ) ? 0xfff : 0x000;
		pulse_output = -( ( accumulator >> 12 ) >= pw ) & 0xfff;

		return waveform_output;
	}

	/**
	* Read OSC3 value.
	*/
	[[ nodiscard ]] sidinline uint8_t readOSC () const noexcept { return uint8_t ( osc3 >> 4 ); }

	/**
	* Read accumulator value.
	*/
	[[ nodiscard ]] sidinline unsigned int readAccumulator () const noexcept { return accumulator; }

	/**
	* Read freq value.
	*/
	[[ nodiscard ]] sidinline unsigned int readFreq () const noexcept { return freq; }

	/**
	* Read test value.
	*/
	[[ nodiscard ]] sidinline bool readTest () const noexcept { return test; }

	/**
	* Read sync value.
	*/
	[[ nodiscard ]] sidinline bool readSync () const noexcept { return sync; }
};

} // namespace reSIDfp
