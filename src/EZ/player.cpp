#include "player.h"

#include "../stringutils.h"

//-----------------------------------------------------------------------------

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

void Player::setRoms ( const void* kernal, const void* basic, const void* character )
{
	engine.setKernal ( (const uint8_t*)kernal );
	engine.setBasic ( (const uint8_t*)basic );
	engine.setChargen ( (const uint8_t*)character );
}
//-----------------------------------------------------------------------------

void Player::setSamplerate ( const int sampleRate )
{
	config.frequency = sampleRate;
}
//-----------------------------------------------------------------------------

bool Player::loadSidFile ( const char* filename )
{
	readyToPlay = false;
	stiEZ = {};

	tune.load ( filename );
	stiEZ.md5 = tune.createMD5New ();

	auto	info = tune.getInfo ();
	if ( ! info )
		return false;

	tuneOverride = overrideSelector.getOverride ( info->path (), info->dataFileName () );

	// Fill basic tune information (global for all songs)
	{
		stiEZ.title = stringutils::extendedASCIItoUTF8 ( info->infoString ( 0 ) );
		stiEZ.author = stringutils::extendedASCIItoUTF8 ( info->infoString ( 1 ) );
		stiEZ.released = stringutils::extendedASCIItoUTF8 ( info->infoString ( 2 ) );

		stiEZ.filename = std::string ( info->path () ) + std::string ( info->dataFileName () );

		stiEZ.numSongs = info->songs ();

		stiEZ.startSong = tuneOverride.startTune ? tuneOverride.startTune : info->startSong ();

		stiEZ.playroutineID = sidID.findPlayerRoutines ( tune.getSidData () );

		stiEZ.c64LoadAddress = info->loadAddr ();
		stiEZ.c64InitAddress = info->initAddr ();
		stiEZ.c64PlayAddress = info->playAddr ();
		stiEZ.c64DataLength = info->c64dataLen ();
	}

	return tune.getStatus ();
}
//-----------------------------------------------------------------------------

bool libsidplayEZ::Player::setTuneNumber ( unsigned int songNo, const bool useFilter )
{
	readyToPlay = false;

	//
	// Apply overrides
	//

	// Start song
	if ( ! songNo && tuneOverride.startTune )
		songNo = tuneOverride.startTune;

	// Select song
	stiEZ.currentSong = tune.selectSong ( songNo );

	auto	info = tune.getInfo ();
	if ( ! info )
		return false;

	// Reset
	config.defaultC64Model = SidConfig::c64_model_t::PAL;
	config.forceC64Model = false;
	config.defaultSidModel = SidConfig::sid_model_t::MOS6581;
	config.forceSidModel = false;

	// Clock
	if ( info->clockSpeed () == SidTuneInfo::clock_t::CLOCK_UNKNOWN && tuneOverride.clock )
	{
		config.defaultC64Model = tuneOverride.clock == 1 ? SidConfig::c64_model_t::PAL : SidConfig::c64_model_t::NTSC;
		config.forceC64Model = true;
	}

	// SID
	if ( tuneOverride.chipModel )
	{
		config.defaultSidModel = tuneOverride.chipModel == 1 ? SidConfig::sid_model_t::MOS6581 : SidConfig::sid_model_t::MOS8580;
		config.forceSidModel = true;
	}

	// Apply config
	config.useFilter = useFilter;
	if ( ! engine.setConfig ( config ) )
		return false;

	// Load the tune
	readyToPlay = engine.loadTune ( &tune );

	if ( ! readyToPlay )
		return false;

	// Fill the info struct for this particular tune
	{
		// Model(s)
		for ( auto i = 0; i < engine.getNumChips (); ++i )
		{
			if ( config.forceSidModel )
				stiEZ.model.emplace_back ( config.defaultSidModel == SidConfig::sid_model_t::MOS8580 ? "8580" : "6581" );
			else
				stiEZ.model.emplace_back ( info->sidModel ( i ) == SidTuneInfo::model_t::SIDMODEL_8580 ? "8580" : "6581" );
		}

		// Clock
		if ( config.forceC64Model )
			stiEZ.clock = config.defaultC64Model == SidConfig::c64_model_t::NTSC ? "NTSC" : "PAL";
		else
			stiEZ.clock = info->clockSpeed () == SidTuneInfo::clock_t::CLOCK_NTSC ? "NTSC" : "PAL";

		// Speed
		const auto& engineInfo = (const SidInfoImpl&)engine.getInfo ();

		stiEZ.speed = engineInfo.speedString ();
	}

	//
	// Attempt to have better sounding SIDs by adjusting filter-range, digi-boost, and combined waveform strength
	// per author with the assumption they worked with the same machine their entire career
	//
	{
		const auto [ profileName, chipProfile ] = chipSelector.getProfile ( info->path (), info->dataFileName () );

		stiEZ.chipProfile = profileName;

		engine.set6581FilterRange ( chipProfile.fltCox );
		engine.set6581FilterCurve ( chipProfile.flt0Dac );
		engine.set6581FilterGain ( chipProfile.fltGain );

		engine.set6581DigiVolume ( chipProfile.digi );

		engine.setCombinedWaveforms ( reSIDfp::CombinedWaveforms ( chipProfile.cwsLevel ), 1.0f );
	}

	// Override chip-profile for Emulation based SID editors (Cheesecutter, GoatTracker, SidWizard etc.)
	{
		if ( stiEZ.model[ 0 ] == "6581" && ! stiEZ.playroutineID.empty () )
		{
			auto oldEmulation = [ this ]
			{
				stiEZ.chipProfile = "Editor uses reSID emulation";

				engine.set6581FilterRange ( 0.5 );
				engine.set6581FilterCurve ( 0.5 );
				engine.set6581FilterGain ( 1.0 );
				engine.set6581DigiVolume ( 1.0 );
				engine.setCombinedWaveforms ( reSIDfp::CombinedWaveforms::STRONG, 1.0 );
			};

			static const std::vector<std::string>	editorsUsingEmulation = {
				"CheeseCutter_", "GoatTracker_V", "SidWizard_", "Hermit/SidWizard_V", "SidFactory_II/",
			};

			for ( const auto& id : editorsUsingEmulation )
				if ( stiEZ.playroutineID[ 0 ].starts_with ( id ) )
					oldEmulation ();
		}
	}

	//
	// Get audio profile for specific 2SID and 3SID, and even some 1SID tunes. Most will be mixed to mono,
	// but we can provide a list where we want a full or narrowed stereo field and bass-adjustment
	//
	{
		const auto audioProfile = audioSelector.getProfile ( info->path (), info->dataFileName () );

		stiEZ.stereoWidth = audioProfile.width;
		stiEZ.bassAdjust = float ( audioProfile.bass );
	}

	return readyToPlay;
}
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
