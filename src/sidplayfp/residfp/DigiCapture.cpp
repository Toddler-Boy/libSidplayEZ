/*
* This file is part of libSidplayEZ, a SID player engine based on libsidplayfp.
*
* Copyright 2026 Michael Hartmann <mike@ultrasid.com>
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

#include "DigiCapture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "DigiMode.h"
#include "MahoneyLevels.h"
#include "Voice.h"

namespace reSIDfp
{

// Smoothing corners per capture mode in Hz, tuned by eye to sit just above
// each technique's real content bandwidth (everything above is stair-step
// artifacts or carrier); DigiMode order
static constexpr std::array<int, 21>	cornerHz =
{
	1500,	// nibble: volume-register players run at a few kHz (CIA-timed NMIs), lower corner than the byte modes
	3000,	// mahoney
	3000,	// freq1
	3000,	// freq2
	3000,	// freq3
	1500,	// pwLo1
	500,	// pwHi1
	1150,	// pwFull1
	500,	// filt1
	1150,	// voice3Out
	500,	// voice1Pwm
	1000,	// covox
	3000,	// carmina (voice 1; voice 2 has its own corner)
	1500,	// escos
	3000,	// output
	3000,	// output2x
	3000,	// output3x
	3000,	// output4x
	3000,	// rawCtrl3
	3000,	// rawPw1
	1500,	// unknown: the buffer carries plain nibble levels
};
static constexpr int	dcCornerHz = 70;
static constexpr int	carminaCornerHz = 500;

// unknown mode: a block with this many changes on one register carries a
// sample stream (~2 kHz); frame-driven music tops out at a few per block
static constexpr int	busyChangesPerBlock = 32;

//-----------------------------------------------------------------------------

// The one-pole alpha for a corner at the active rate: 1 - e^(-2 pi f / fs)
static float alphaFor ( const int hz, const double sampleRate ) noexcept
{
	return float ( 1.0 - std::exp ( -2.0 * std::numbers::pi * double ( hz ) / sampleRate ) );
}
//-----------------------------------------------------------------------------

DigiCapture::DigiCapture ()
{
	setSamplingRate ( 44100.0 );
}
//-----------------------------------------------------------------------------

void DigiCapture::setMode ( const DigiMode newMode ) noexcept
{
	mode = newMode;
	alpha = alphaFor ( cornerHz[ size_t ( mode ) ], sampleRate );

	lp1 = lp2 = dc = carminaLp = 0.0f;

	rates = {};
	prevPoke.fill ( 0 );
	blockChanges.fill ( 0 );
	blockCountdown = blockSamples;
}
//-----------------------------------------------------------------------------

void DigiCapture::setScanMode ( const DigiMode newMode ) noexcept
{
	switch ( newMode )
	{
		case DigiMode::voice3Out:	setMode ( DigiMode::rawCtrl3 );	break;
		case DigiMode::voice1Pwm:	setMode ( DigiMode::rawPw1 );	break;
		default:					setMode ( newMode );			break;
	}
}
//-----------------------------------------------------------------------------

void DigiCapture::setSamplingRate ( const double samplingFrequency ) noexcept
{
	sampleRate = samplingFrequency;

	alpha = alphaFor ( cornerHz[ size_t ( mode ) ], sampleRate );
	dcAlpha = alphaFor ( dcCornerHz, sampleRate );
	carminaAlpha = alphaFor ( carminaCornerHz, sampleRate );

	blockSamples = std::max ( 1, int ( sampleRate / 60.0 ) );
	blockCountdown = blockSamples;
}
//-----------------------------------------------------------------------------

template <bool is6581>
int8_t DigiCapture::capture ( const uint8_t* lastpoke, const Voice<is6581>* voice, const int mixedSample ) noexcept
{
	// The raw display level per capture mode; the smoothing corner comes from
	// the per-mode table, re-pitched to the sampling rate
	auto	level = 0.0f;

	switch ( mode )
	{
		default:
		case DigiMode::nibble:
			level = float ( ( ( lastpoke[ 0x18 ] & 0x0F ) << 4 ) - 128 );
			break;

		case DigiMode::mahoney:
			level = float ( ( is6581 ? mahoney6581Levels : mahoney8580Levels )[ lastpoke[ 0x18 ] ] );
			break;

		case DigiMode::freq1:
			level = float ( lastpoke[ 0x01 ] - 128 );
			break;

		case DigiMode::freq2:
			level = float ( lastpoke[ 0x08 ] - 128 );
			break;

		case DigiMode::freq3:
			level = float ( lastpoke[ 0x0f ] - 128 );
			break;

		case DigiMode::pwHi1:
			// Cyberbrain 4-channel mixer: the full 8-bit sum lands in
			// PW-hi (hardware uses 4 bits, the shadow keeps all 8)
			level = float ( lastpoke[ 0x03 ] - 128 );
			break;

		case DigiMode::pwFull1:
			// StreetTuff PWM: an inverted 8-bit sample spread across the
			// full 12-bit pulse width (high nibble in PW-hi, low nibble in
			// PW-lo's top bits), reassembled from both shadow bytes
			level = 127.0f - float ( ( ( lastpoke[ 0x03 ] & 0x0F ) << 4 ) | ( lastpoke[ 0x02 ] >> 4 ) );
			break;

		case DigiMode::filt1:
			// Silas Warner speech: sample bytes time the flips of the
			// voice 1 filter-routing bit, each edge steps the mixer DC;
			// the low-pass demodulates the edge stream into the waveform
			level = ( lastpoke[ 0x17 ] & 0x01 ) ? 127.0f : -128.0f;
			break;

		case DigiMode::pwLo1:
			// 16 kHz Censor bit stream riding voice 1 PW-lo (full-scale
			// swings, test-bit retriggered), amplitude in the envelope;
			// the low-pass demodulates it into the waveform
			level = float ( lastpoke[ 0x02 ] - 128 ) * voice[ 0 ].getEnvLevel ();
			break;

		case DigiMode::rawCtrl3:
			level = float ( lastpoke[ 0x12 ] - 128 );
			break;

		case DigiMode::rawPw1:
			level = float ( lastpoke[ 0x03 ] - 128 );
			break;

		case DigiMode::voice1Pwm:
			// The mean pulse level over a carrier period: the fraction
			// of the period spent high (only 4 upper bits of pulse width),
			// scaled by the live envelope; the low corner suppresses
			// the audible-band carrier
			level = float ( ( ( lastpoke[ 0x03 ] & 0x0F ) << 4 ) - 128 ) * voice[ 0 ].getEnvLevel ();
			break;

		case DigiMode::voice3Out:
			// Frozen osc3 = digi; OSC3 readback x live envelope
			if ( lastpoke[ 0x0E ] == 0 && lastpoke[ 0x0F ] == 0 )
				level = float ( voice[ 2 ].waveformGenerator.readOSC () - 128 ) * voice[ 2 ].getEnvLevel ();
			break;

		case DigiMode::carmina:
			// Carmina Burana: voice 1 plays freq-latch samples (the freq
			// byte IS the DAC input, test kicks clock it, a HELD test bit
			// mutes the stream during fade-ins), voice 2 adds a
			// pulse-position stream (kicks timed by a second CIA clock, no
			// register carries a level) that only the voice output shows.
			// Uncorrelated signals, voice 2's envelope stays at half scale,
			// so the plain sum fits the display range
			if ( ! ( lastpoke[ 0x04 ] & 0x08 ) )
				level = float ( lastpoke[ 0x01 ] - 128 ) * voice[ 0 ].getEnvLevel ();
			carminaLp += carminaAlpha * ( float ( voice[ 1 ].waveformGenerator.readOSC () - 128 ) * voice[ 1 ].getEnvLevel () - carminaLp );
			level = ( level + carminaLp ) * 0.85f;
			break;

		case DigiMode::escos:
			// Escos: a seven-channel 1-bit impulse synth on voice 3 (CIA
			// timers fire test kicks at note-period intervals, no register
			// carries a level); only the voice output shows the impulse
			// train, the low-pass turns it into the chord waveform
			level = float ( voice[ 2 ].waveformGenerator.readOSC () - 128 ) * voice[ 2 ].getEnvLevel ();
			break;

		case DigiMode::output:
			// FRODIGI school: all three oscillators and the master volume
			// resynthesize the audio at a low sample rate, no register
			// carries the sample, the final mix IS the digi (which only
			// spans ~12 bits, hence the hotter scale; the clamp below
			// catches the rest)
			level = float ( mixedSample ) * ( 1.0f / 64.0f );
			break;

		case DigiMode::output2x:
			// output for quiet mixes: hotter scales, the clamp eats overshoots
			level = float ( mixedSample ) * ( 2.0f / 64.0f );
			break;

		case DigiMode::output3x:
			level = float ( mixedSample ) * ( 3.0f / 64.0f );
			break;

		case DigiMode::output4x:
			level = float ( mixedSample ) * ( 4.0f / 64.0f );
			break;

		case DigiMode::unknown:
			// No established technique: count the changes of every write
			// register per block while the buffer carries the plain nibble
			// level, so the regular wiggler detection stays intact
			for ( auto i = 0; i < watchedRegs; ++i )
				if ( lastpoke[ i ] != prevPoke[ i ] )
				{
					prevPoke[ i ] = lastpoke[ i ];
					++blockChanges[ i ];
				}

			if ( --blockCountdown <= 0 )
			{
				blockCountdown = blockSamples;

				for ( auto i = 0; i < watchedRegs; ++i )
				{
					rates.maxPerBlock[ i ] = std::max ( rates.maxPerBlock[ i ], blockChanges[ i ] );
					rates.busyBlocks[ i ] += blockChanges[ i ] >= busyChangesPerBlock;
					blockChanges[ i ] = 0;
				}
			}

			level = float ( ( ( lastpoke[ 0x18 ] & 0x0F ) << 4 ) - 128 );
			break;

		case DigiMode::covox:
			// Test-bit speech on voice 1: Voice Master density streams
			// (~12.6 kHz, ~100 Hz volume track) and Reynolds sign PCM
			// (~7 kHz, block volume) both demodulate through the low-pass.
			// The players leave voice 1 freq at 0 and the envelope at full,
			// running music does not; the bit comes from the ctrl shadow
			if ( lastpoke[ 0x00 ] == 0 && lastpoke[ 0x01 ] == 0 )
				level = ( lastpoke[ 0x04 ] & 0x08 ? 127.0f : -128.0f ) * float ( lastpoke[ 0x18 ] & 0x0F ) * ( 1.0f / 15.0f );
			break;
	}

	if ( smooth ) [[ likely ]]
	{
		lp1 += alpha * ( level - lp1 );
		lp2 += alpha * ( lp1 - lp2 );

		// Re-center every mode (idle levels, off-center 4-bit streams)
		dc += dcAlpha * ( lp2 - dc );
	}
	else
	{
		// Raw capture for measurement: the level passes through untouched
		lp2 = level;
		dc = 0.0f;
	}

	const auto	v = int ( lp2 - dc );
	return int8_t ( std::clamp ( v, -128, 127 ) );
}
//-----------------------------------------------------------------------------

template int8_t DigiCapture::capture ( const uint8_t*, const Voice<true>*, int ) noexcept;
template int8_t DigiCapture::capture ( const uint8_t*, const Voice<false>*, int ) noexcept;

}
