#pragma once
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

#include <memory>
#include <algorithm>
#include <limits>

namespace reSIDfp
{
	typedef enum { MOS6581 = 1, MOS8580 } ChipModel;
	typedef enum { WEAK, AVERAGE, STRONG } CombinedWaveforms;
}

#include "Dac.h"
#include "Filter.h"

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
	FLT		filter;

	// External filter that provides high-pass and low-pass filtering to adjust sound tone slightly
	ExternalFilter	externalFilter;

	// Table of waveforms
	std::vector<int16_t>	waveTable;
	std::vector<int16_t>	pulldownTable;

	// Resampler used by audio generation code
	TwoPassSincResampler	resampler;

	static constexpr int	numVoices = 3;

	// SID voices
	Voice	voice[ numVoices ];

	// Used to amplify the output by x/2 to get an adequate playback volume
	int	scaleFactor;

	// Used to determine if the filter ever gets used during playback
	uint8_t	filterUsage;

	// Time to live for the last written value
	int	busValueTtl;

	// Current chip model's bus value TTL
	int	modelTTL;

	// Time until #voiceSync must be run.
	unsigned int nextVoiceSync;

	// Currently active chip model.
	ChipModel model;

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

private:
	/**
	* Calculate the number of cycles according to current parameters
	* that it takes to reach sync
	*
	* @param sync whether to do the actual voice synchronization
	*/
	sidinline void voiceSync ( const bool sync )
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

		for ( auto i = 0; i < numVoices; i++ )
		{
			auto&		wave = voice[ i ].waveformGenerator;
			const auto	freq = wave.readFreq ();

			if ( wave.readTest () || freq == 0 || ! voice[ ( i + 1 ) % 3 ].waveformGenerator.readSync () ) [[ likely ]]
				continue;

			const auto	accumulator = wave.readAccumulator ();
			const auto	thisVoiceSync = ( ( 0x7FFFFF - accumulator ) & 0xFFFFFF ) / freq + 1;

			if ( thisVoiceSync < nextVoiceSync )
				nextVoiceSync = thisVoiceSync;
		}
	}

	void recalculateDACs ()
	{
		constexpr auto	ENV_DAC_BITS = 8u;
		constexpr auto	OSC_DAC_BITS = 12u;

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
			Dac	dacBuilder ( OSC_DAC_BITS, dacLeakFactor );
			dacBuilder.kinkedDac ( model == MOS6581 );

			const auto	offset = dacBuilder.getOutput ( 0x7ff, model == MOS6581 );// model == MOS6581 ? OFFSET_6581 : OFFSET_8580 );

			for ( auto i = 0u; i < ( 1 << OSC_DAC_BITS ); i++ )
				oscDAC[ i ] = float ( dacBuilder.getOutput ( i ) - offset );
		}
	}

public:
	SID ()
	{
		waveTable = WaveformCalculator::buildWaveTable ();

		reset ();
		setChipModel ( MOS8580 );
	}

	/**
	* Set chip model.
	*
	* @param model chip model to use
	*/
	void setChipModel ( ChipModel _model )
	{
		constexpr auto	BUS_TTL_6581 = 0x01D00;
		constexpr auto	BUS_TTL_8580 = 0xA2000;

		model = _model;

		if constexpr ( std::is_same_v<FLT, Filter6581<true>> || std::is_same_v<FLT, Filter6581<false>> )
		{
			scaleFactor = 3;
			modelTTL = BUS_TTL_6581;
		}
		else if constexpr ( std::is_same_v<FLT, Filter8580<true>> || std::is_same_v<FLT, Filter8580<false>> )
		{
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

	/**
	* Get currently emulated chip model.
	*/
	ChipModel getChipModel () const { return model; }

	/**
	* Set combined waveforms strength.
	*/
	void setCombinedWaveforms ( CombinedWaveforms cws, const float threshold )
	{
		WaveformCalculator::buildPulldownTable ( pulldownTable, model == MOS6581, cws, threshold );

		for ( auto& vce : voice )
			vce.waveformGenerator.setPulldownModels ( pulldownTable );
	}

	/**
	* Set DAC leakage
	*/
	void setDacLeakage ( const double leakage )
	{
		dacLeakage = leakage;
		recalculateDACs ();
	}

	/**
	* Set Voice DC drift
	*/
	void setVoiceDCDrift ( const double drift )
	{
		voiceDCDrift = drift;

		if constexpr ( std::is_same_v<FLT, Filter6581<true>> || std::is_same_v<FLT, Filter6581<false>> )
			filter.setVoiceDCDrift ( drift );
	}

	/**
	* SID reset.
	*/
	void reset ()
	{
		for ( auto& vce : voice )
			vce.reset ();

		filter.reset ();
		externalFilter.reset ();

		resampler.reset ();

		filterUsage = 0;
		busValue = 0;
		busValueTtl = 0;
		voiceSync ( false );
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
	[[ nodiscard ]] sidinline uint8_t read ( int offset )
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

	/**
	* Write registers.
	*
	* @param offset chip register to write
	* @param value value to write
	*/
	sidinline void write ( int offset, uint8_t value )
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
			case 0x15:	filter.writeFC_LO ( value ); 									break;	// Filter cut off frequency (bits #0-#2)
			case 0x16:	filter.writeFC_HI ( value ); 									break;	// Filter cut off frequency (bits #3-#10)
			case 0x17:																			// Filter control
			{
				filter.writeRES_FILT ( value );
				filterUsage |= value;
			}
			break;
			case 0x18:	filter.writeMODE_VOL ( value );									break;	// Volume and filter modes

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
	void setSamplingParameters ( double clockFrequency, double samplingFrequency )
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
	sidinline int clock ( unsigned int cycles, int16_t* buf )
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

		auto output = [ this ] () -> int
		{
			const auto	o1 = voice[ 0 ].output ( voice[ 2 ].waveformGenerator );
			const auto	o2 = voice[ 1 ].output ( voice[ 0 ].waveformGenerator );
			const auto	o3 = voice[ 2 ].output ( voice[ 1 ].waveformGenerator );

			if constexpr ( std::is_same_v<FLT, Filter6581<true>> || std::is_same_v<FLT, Filter6581<false>> )
			{
				const auto	env1 = voice[ 0 ].envelopeGenerator.output ();
				const auto	env2 = voice[ 1 ].envelopeGenerator.output ();
				const auto	env3 = voice[ 2 ].envelopeGenerator.output ();

				const auto	input = int ( filter.clock ( o1, o2, o3, env1, env2, env3 ) );
				return externalFilter.clock ( input + INT16_MIN );
			}
			else if constexpr ( std::is_same_v < FLT, Filter8580<true>> || std::is_same_v < FLT, Filter8580<false>> )
			{
 				const auto	input = int ( filter.clock ( o1, o2, o3 ) );
 				return externalFilter.clock ( input + INT16_MIN );
 			}
		};

		auto    s = 0;
		while ( cycles )
		{
			if ( auto delta_t = std::min ( nextVoiceSync, cycles ); delta_t > 0 ) [[ likely ]]
			{
				for ( auto i = 0u; i < delta_t; i++ )
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
						buf[ s++ ] = int16_t ( resampler.output ( scaleFactor ) );
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
	void setFilter6581Curve ( [[ maybe_unused ]] double filterCurve )
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setFilterCurve ( filterCurve );
	}

	/**
	* Set filter range parameter for 6581 model
	*
	* @see Filter6581::setFilterRange(double)
	*/
	void setFilter6581Range ( [[ maybe_unused ]] double adjustment )
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setFilterRange ( adjustment );
	}

	/**
	* Set filter gain parameter for 6581 model
	*
	* @see Filter6581::setFilterGain(double)
	*/
	void setFilter6581Gain ( [[ maybe_unused ]] double adjustment )
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> )
			filter.setFilterGain ( adjustment );
	}

	/**
	* Set filter digi volume for 6581 model
	*
	* @see Filter6581::setDigitVolume(double)
	*/
	void setFilter6581Digi ( [[ maybe_unused ]] double adjustment )
	{
		if constexpr ( std::is_same_v<FLT, Filter6581<true>> || std::is_same_v<FLT, Filter6581<false>> )
			filter.setDigiVolume ( adjustment );
	}

	/**
	* Set filter curve parameter for 8580 model.
	*
	* @see Filter8580::setFilterCurve(double)
	*/
	void setFilter8580Curve ( [[ maybe_unused ]] double filterCurve )
	{
		if constexpr ( std::is_same_v<FLT, Filter8580<true>> )
			filter.setFilterCurve ( filterCurve );
	}

	float getEnvLevel ( int voiceNo ) const		{	return voice[ voiceNo ].getEnvLevel (); }
	bool wasFilterUsed () const					{	return filterUsage & 0x0F;		}
};
//-----------------------------------------------------------------------------

} // namespace reSIDfp
