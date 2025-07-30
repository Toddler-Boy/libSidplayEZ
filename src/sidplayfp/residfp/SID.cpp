/*
* This file is part of libsidplayfp, a SID player engine.
*
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

#include "SID.h"

#include <limits>

#include "Dac.h"
#include "Filter6581.h"
#include "Filter8580.h"
#include "WaveformCalculator.h"
#include "resample/TwoPassSincResampler.h"

namespace reSIDfp
{

constexpr auto	ENV_DAC_BITS = 8u;
constexpr auto	OSC_DAC_BITS = 12u;

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
//constexpr auto	OFFSET_6581 = 0x380u;
//constexpr auto	OFFSET_8580 = 0x9c0u;

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
//@{
constexpr auto	BUS_TTL_6581 = 0x01D00;
constexpr auto	BUS_TTL_8580 = 0xA2000;
//@}

//-----------------------------------------------------------------------------

SID::SID ()
{
	waveTable = WaveformCalculator::buildWaveTable ();

	reset ();
	setChipModel ( MOS8580 );
}
//-----------------------------------------------------------------------------

void SID::setChipModel ( ChipModel _model )
{
	model = _model;

	if ( model == MOS6581 )
	{
		filter = &filter6581;
		scaleFactor = 3;
		modelTTL = BUS_TTL_6581;
	}
	else
	{
		filter = &filter8580;
		scaleFactor = 5;
		modelTTL = BUS_TTL_8580;
	}

	recalculateDACs ();

	// set voice tables
	for ( auto& vce : voice )
	{
		vce.setEnvDAC ( envDAC );
		vce.setWavDAC ( oscDAC );
		vce.waveformGenerator.setModel ( model == MOS6581 );
		vce.waveformGenerator.setWaveformModels ( waveTable );
	}

	setCombinedWaveforms ( CombinedWaveforms::AVERAGE, 1.0f );
}
//-----------------------------------------------------------------------------

void SID::setCombinedWaveforms ( CombinedWaveforms cws, const float threshold )
{
	WaveformCalculator::buildPulldownTable ( pulldownTable, model == MOS6581, cws, threshold );

	for ( auto& vce : voice )
		vce.waveformGenerator.setPulldownModels ( pulldownTable );
}
//-----------------------------------------------------------------------------

void SID::setDacLeakage ( const double leakage )
{
	dacLeakage = leakage;
	recalculateDACs ();
}
//-----------------------------------------------------------------------------

void SID::setVoiceDCDrift ( const double drift )
{
	voiceDCDrift = drift;

	filter6581.setVoiceDCDrift ( drift );
}
//-----------------------------------------------------------------------------

void SID::recalculateDACs ()
{
	constexpr auto	MOSFET_LEAKAGE_6581 = 0.0075;
	constexpr auto	MOSFET_LEAKAGE_8580 = 0.0035;

	const auto	dacLeakFactor = ( model == MOS6581 ? MOSFET_LEAKAGE_6581 : MOSFET_LEAKAGE_8580 ) * dacLeakage;

	// calculate envelope DAC table
	{
		Dac	dacBuilder ( ENV_DAC_BITS, dacLeakFactor );
		dacBuilder.kinkedDac ( model == MOS6581 );

		for ( auto i = 0u; i < ( 1 << ENV_DAC_BITS ); i++ )
			envDAC[ i ] = float ( dacBuilder.getOutput ( i ) );
	}

	// calculate oscillator DAC table
	{
		Dac dacBuilder ( OSC_DAC_BITS, dacLeakFactor );
		dacBuilder.kinkedDac ( model == MOS6581 );

		const auto	offset = dacBuilder.getOutput ( 0x7ff, model == MOS6581 );// model == MOS6581 ? OFFSET_6581 : OFFSET_8580 );

		for ( auto i = 0u; i < ( 1 << OSC_DAC_BITS ); i++ )
			oscDAC[ i ] = float ( dacBuilder.getOutput ( i ) - offset );
	}
}
//-----------------------------------------------------------------------------

void SID::reset ()
{
	for ( auto& vce : voice )
		vce.reset ();

	filter6581.reset ();
	filter8580.reset ();
	externalFilter.reset ();

	resampler.reset ();

	busValue = 0;
	busValueTtl = 0;
	voiceSync ( false );
}
//-----------------------------------------------------------------------------

uint8_t SID::read ( int offset )
{
	switch ( offset )
	{
		case 0x19: // X value of paddle
			busValue = 0xff;
			busValueTtl = modelTTL;
			break;

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
//-----------------------------------------------------------------------------

void SID::write ( int offset, uint8_t value )
{
	busValue = value;
	busValueTtl = modelTTL;

	switch ( offset )
	{
		case 0x00:	voice[ 0 ].waveformGenerator.writeFREQ_LO ( value );			break;	// Voice #1 frequency (Low-byte)
		case 0x01:	voice[ 0 ].waveformGenerator.writeFREQ_HI ( value );			break;	// Voice #1 frequency (High-byte)
		case 0x02:	voice[ 0 ].waveformGenerator.writePW_LO ( value );				break;	// Voice #1 pulse width (Low-byte)
		case 0x03:	voice[ 0 ].waveformGenerator.writePW_HI ( value );				break;	// Voice #1 pulse width (bits #8-#15)
		case 0x04:	voice[ 0 ].writeCONTROL_REG ( value );							break;	// Voice #1 control register
		case 0x05:	voice[ 0 ].envelopeGenerator.writeATTACK_DECAY ( value );		break;	// Voice #1 Attack and Decay length
		case 0x06:	voice[ 0 ].envelopeGenerator.writeSUSTAIN_RELEASE ( value );	break;	// Voice #1 Sustain volume and Release length
		case 0x07:	voice[ 1 ].waveformGenerator.writeFREQ_LO ( value );			break;	// Voice #2 frequency (Low-byte)
		case 0x08:	voice[ 1 ].waveformGenerator.writeFREQ_HI ( value );			break;	// Voice #2 frequency (High-byte)
		case 0x09:	voice[ 1 ].waveformGenerator.writePW_LO ( value );				break;	// Voice #2 pulse width (Low-byte)
		case 0x0a:	voice[ 1 ].waveformGenerator.writePW_HI ( value );				break;	// Voice #2 pulse width (bits #8-#15)
		case 0x0b:	voice[ 1 ].writeCONTROL_REG ( value );							break;	// Voice #2 control register
		case 0x0c:	voice[ 1 ].envelopeGenerator.writeATTACK_DECAY ( value );		break;	// Voice #2 Attack and Decay length
		case 0x0d:	voice[ 1 ].envelopeGenerator.writeSUSTAIN_RELEASE ( value );	break;	// Voice #2 Sustain volume and Release length
		case 0x0e:	voice[ 2 ].waveformGenerator.writeFREQ_LO ( value );			break;	// Voice #3 frequency (Low-byte)
		case 0x0f:	voice[ 2 ].waveformGenerator.writeFREQ_HI ( value );			break;	// Voice #3 frequency (High-byte)
		case 0x10:	voice[ 2 ].waveformGenerator.writePW_LO ( value );				break;	// Voice #3 pulse width (Low-byte)
		case 0x11:	voice[ 2 ].waveformGenerator.writePW_HI ( value );				break;	// Voice #3 pulse width (bits #8-#15)
		case 0x12:	voice[ 2 ].writeCONTROL_REG ( value );							break;	// Voice #3 control register
		case 0x13:	voice[ 2 ].envelopeGenerator.writeATTACK_DECAY ( value );		break;	// Voice #3 Attack and Decay length
		case 0x14:	voice[ 2 ].envelopeGenerator.writeSUSTAIN_RELEASE ( value );	break;	// Voice #3 Sustain volume and Release length
		case 0x15:	filter->writeFC_LO ( value ); 									break;	// Filter cut off frequency (bits #0-#2)
		case 0x16:	filter->writeFC_HI ( value ); 									break;	// Filter cut off frequency (bits #3-#10)
		case 0x17:	filter->writeRES_FILT ( value );								break;	// Filter control
		case 0x18:	filter->writeMODE_VOL ( value );								break;	// Volume and filter modes

		default:
			break;
	}

	// Update voicesync just in case
	voiceSync ( false );
}
//-----------------------------------------------------------------------------

void SID::setSamplingParameters ( double clockFrequency, double samplingFrequency )
{
	externalFilter.setClockFrequency ( clockFrequency );

	resampler.setup ( clockFrequency, samplingFrequency );
}
//-----------------------------------------------------------------------------

} // namespace reSIDfp
