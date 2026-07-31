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
	void set6581VoiceDrift ( const double drift )	{ engine.set6581VoiceDCDrift ( drift ); }
	void set6581LeakageRate ( const double rate )	{ engine.set6581LeakageRate ( rate ); }

	void set6581Filter_uCoxAndCap ( const double uCox, const bool oldCap )	{ engine.set6581Filter_uCoxAndCap ( uCox, oldCap ); }
	void set6581FilterCurve ( const double value )							{ engine.set6581FilterCurve ( value ); }
	void set6581FilterGain ( const double value )							{ engine.set6581FilterGain ( value ); }
	void set6581FilterSaturation ( const double value )						{ engine.set6581FilterSaturation ( value ); }
	void set6581FilterBandpassWidthOffset ( const double value )			{ engine.set6581FilterBandpassWidthOffset ( value ); }
	void set6581DigiVolume ( const double value )							{ engine.set6581DigiVolume ( value ); }
	void setCombinedWaveforms ( const reSIDfp::CombinedWaveforms cws, const float threshold )	{ engine.setCombinedWaveforms ( cws, threshold ); }
	void set6581SawPulseUltra ( const bool value )							{ engine.set6581SawPulseUltra ( value ); }

	[[ nodiscard ]] unsigned int getEmulatedTimeMs () const { return engine.timeMs (); }

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
