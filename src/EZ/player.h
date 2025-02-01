#pragma once

#include "../player.h"
#include "sidid.h"
#include "chip-selector.h"
#include "SidTuneInfoEZ.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

class Player final
{
public:
	bool loadSidIDConfig ( const char* filename ) { return sidID.loadSidIDConfig ( filename ); }
	void setChipProfileMap ( const ChipSelector::profileMap& map ) { chipSelector.setProfiles ( map ); }

	void setRoms ( const void* kernal, const void* basic, const void* character );

	void setSamplerate ( const int _sampleRate );
	bool isReadyToPlay () const { return readyToPlay; }

	bool loadSidFile ( const char* filename );
	bool setTuneNumber (const unsigned int songNo = 0 );
	uint32_t runEmulation ( float* dst, uint32_t lengthWanted )		{	return engine.play ( dst, lengthWanted );		}
	bool getSidStatus ( int sidNum, uint8_t regs[ 32 ] )			{	return engine.getSidStatus ( sidNum, regs );	}
	uint16_t getInterruptCycles () const							{	return engine.getInterruptCycles ();			}

	[[ nodiscard ]] int getNumChips () const { return engine.getNumChips (); }

	[[ nodiscard ]] const SidTuneInfoEZ& getFileInfo () const	{	return stiEZ;	}
	[[ nodiscard ]] const SidTune& getSidTune () const { return tune; }

	void setDacLeakage ( const double leakage )		{ engine.setDacLeakage ( leakage ); }
	void set6581VoiceDrift ( const double drift )	{ engine.set6581VoiceDCDrift ( drift ); }

	[[ nodiscard ]] unsigned int getEmulatedTimeMs () const { return engine.timeMs (); }

private:
	bool	readyToPlay = false;

	ChipSelector	chipSelector;

	libsidplayfp::Player	engine;

	SidTune		tune;
	SidConfig	config;

	sidid		sidID;

	SidTuneInfoEZ	stiEZ;
};
//-----------------------------------------------------------------------------

}
