#pragma once

#include <memory>

#include "../player.h"

#include "shared-config.h"
#include "SidTuneInfoEZ.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

class Player final
{
public:
	// All parsed configuration comes from one shared, immutable object: parse it
	// once via SharedPlayerConfig::load, then hand the same pointer to every Player
	void setSharedConfig ( std::shared_ptr<const SharedPlayerConfig> config ) { sharedConfig = std::move ( config ); }

	const OverrideSelector::overrideMap& getAllTuneOverrides () const
	{
		static const OverrideSelector::overrideMap	empty;
		return sharedConfig != nullptr ? sharedConfig->overrideSelector.getAllOverrides () : empty;
	}

	void setRoms ( const void* kernal, const void* basic, const void* character );

	void setSamplerate ( const int _sampleRate );
	bool isReadyToPlay () const { return readyToPlay; }

	bool loadSidFile ( const char* filename );

	// The loader supplies the bytes for a name that isn't a real file (data
	// shipped inside an archive); the name still flows into path-based lookups
	bool loadSidFile ( SidTune::LoaderFunc loader, const char* filename );

	bool setTuneNumber ( const unsigned int songNo = 0, const bool useFilter = true );
	// dstL sets the length; dstR is empty for mono. digiBuffers: one per chip (getNumChips ()), or empty
	uint32_t runEmulation ( std::span<float> dstL, std::span<float> dstR, std::span<const std::span<int8_t>> digiBuffers )	{	return engine.play ( dstL, dstR, digiBuffers );		}
	bool getSidStatus ( int sidNum, uint8_t regs[ 32 ] )			{	return engine.getSidStatus ( sidNum, regs );	}
	bool getDigiWriteRates ( int sidNum, reSIDfp::DigiCapture::WriteRates& rates )	{	return engine.getDigiWriteRates ( sidNum, rates );	}
	uint16_t getInterruptCycles () const							{	return engine.getInterruptCycles ();			}

	[[ nodiscard ]] int getNumChips () const { return engine.getNumChips (); }

	/**
	* Check whether an illegal opcode has halted the CPU. Poll after setTuneNumber ()
	* and after every runEmulation (), which both stop early once it is set.
	*/
	[[ nodiscard ]] bool isJammed () const { return engine.isJammed (); }

	[[ nodiscard ]] const SidTuneInfoEZ& getFileInfo () const	{	return stiEZ;	}
	[[ nodiscard ]] const SidTune& getSidTune () const { return tune; }

	void setDacLeakage ( const double leakage )		{ engine.setDacLeakage ( leakage ); }
	void setExternalFilterResistance ( const double ohms )	{ engine.setExternalFilterResistance ( ohms ); }
	void set6581VoiceDrift ( const double drift )	{ engine.set6581VoiceDCDrift ( drift ); }
	void set6581WaveDCOffset ( const double value )	{ engine.set6581WaveDCOffset ( value ); }
	void set6581LeakageRate ( const double rate )	{ engine.set6581LeakageRate ( rate ); }
	void setTodPowerOnSeed ( const uint32_t seed )	{ engine.setTodPowerOnSeed ( seed ); }

	void set6581Filter_uCoxAndCap ( const double uCox, const bool oldCap )	{ engine.set6581Filter_uCoxAndCap ( uCox, oldCap ); }
	void set6581FilterCurve ( const double value )							{ engine.set6581FilterCurve ( value ); }
	void set6581FilterGain ( const double value )							{ engine.set6581FilterGain ( value ); }
	void set6581FilterSaturation ( const double value )						{ engine.set6581FilterSaturation ( value ); }
	void set6581FilterResonance ( const double value )						{ engine.set6581FilterResonance ( value ); }
	void set6581ExtInDC ( const double value )								{ engine.set6581ExtInDC ( value ); }
	void set6581VoiceDCBias ( const double value )							{ engine.set6581VoiceDCBias ( value ); }
	void set6581CombinedWaveforms ( const reSIDfp::CombinedWaveforms cws, const float threshold )	{ engine.set6581CombinedWaveforms ( cws, threshold ); }
	void set8580CombinedWaveforms ( const reSIDfp::CombinedWaveforms cws, const float threshold )	{ engine.set8580CombinedWaveforms ( cws, threshold ); }
	void setDigiCapture ( const reSIDfp::DigiMode mode )					{ engine.setDigiCapture ( mode ); }
	void setDigiScan ( const reSIDfp::DigiMode mode )						{ engine.setDigiScan ( mode ); }
	void setDigiSmoothing ( const bool enable )								{ engine.setDigiSmoothing ( enable ); }
	void set6581SawPulseUltra ( const bool value )							{ engine.set6581SawPulseUltra ( value ); }

	[[ nodiscard ]] unsigned int getEmulatedTimeMs () const { return engine.timeMs (); }

	// The complete emulation state between two runEmulation () calls; restore
	// needs a player that loaded and initialised the same tune with the same config
	[[ nodiscard ]] bool saveState ( std::vector<uint8_t>& out )			{	return engine.saveState ( out );	}
	[[ nodiscard ]] bool restoreState ( std::span<const uint8_t> in )	{	return engine.restoreState ( in );	}

private:
	// Everything after the tune bytes are in, shared by both loadSidFile flavours
	bool finishLoad ();

	bool	readyToPlay = false;

	std::shared_ptr<const SharedPlayerConfig>	sharedConfig;

	libsidplayfp::Player	engine;

	OverrideSelector::overrides	tuneOverride;

	SidTune		tune;
	SidConfig	config;

	SidTuneInfoEZ	stiEZ;
};
//-----------------------------------------------------------------------------

}
