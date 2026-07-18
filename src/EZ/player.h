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
	bool setTuneNumber ( const unsigned int songNo = 0, const bool useFilter = true );
	uint32_t runEmulation ( float* dstL, float* dstR, int8_t** digiBuffers, uint32_t lengthWanted )	{	return engine.play ( dstL, dstR, digiBuffers, lengthWanted );		}
	bool getSidStatus ( int sidNum, uint8_t regs[ 32 ] )			{	return engine.getSidStatus ( sidNum, regs );	}
	uint16_t getInterruptCycles () const							{	return engine.getInterruptCycles ();			}

	[[ nodiscard ]] int getNumChips () const { return engine.getNumChips (); }

	[[ nodiscard ]] const SidTuneInfoEZ& getFileInfo () const	{	return stiEZ;	}
	[[ nodiscard ]] const SidTune& getSidTune () const { return tune; }

	void setDacLeakage ( const double leakage )		{ engine.setDacLeakage ( leakage ); }
	void set6581VoiceDrift ( const double drift )	{ engine.set6581VoiceDCDrift ( drift ); }
	void set6581LeakageRate ( const double rate )	{ engine.set6581LeakageRate ( rate ); }

	[[ nodiscard ]] unsigned int getEmulatedTimeMs () const { return engine.timeMs (); }

private:
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
