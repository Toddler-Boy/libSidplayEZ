#include <algorithm>
#include <cmath>
#include <string_view>

#include "player.h"

#include "../sidplayfp/SidTuneInfo.h"
#include "../stringutils.h"

//-----------------------------------------------------------------------------

// The audible fields in fixed order, hundredths composed as scaled integers,
// so the text never depends on locale or float formatting (consumers hash it).
// Fields at their default are omitted: a new field is invisible until a
// profile actually sets it, and an unprofiled tune serializes empty
static std::string describeAppliedSettings ( const libsidplayEZ::ChipProfileSelector::settings& s )
{
	const libsidplayEZ::ChipProfileSelector::settings	defaults;

	std::string	text;

	auto field = [ &text ] ( const char* name, const long value, const long defaultValue )
	{
		if ( value == defaultValue )
			return;

		if ( ! text.empty () )
			text += ' ';
		text += name + ( "=" + std::to_string ( value ) );
	};

	auto centi = [] ( const double v ) { return std::lround ( v * 100.0 ); };

	field ( "cap", s.fltCapOld, defaults.fltCapOld );
	field ( "dac", centi ( s.flt0Dac ), centi ( defaults.flt0Dac ) );
	field ( "gain", centi ( s.fltGain ), centi ( defaults.fltGain ) );
	field ( "sat", centi ( s.fltSaturation ), centi ( defaults.fltSaturation ) );
	field ( "bpw", centi ( s.fltBandpassWidthOffset ), centi ( defaults.fltBandpassWidthOffset ) );
	field ( "digi", centi ( s.digi ), centi ( defaults.digi ) );
	field ( "leak", centi ( s.leakageRate ), centi ( defaults.leakageRate ) );
	field ( "cws", s.cwsLevel, defaults.cwsLevel );
	field ( "ultra", s.cwsSawPulseUltra, defaults.cwsSawPulseUltra );

	return text;
}
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

	if ( sharedConfig == nullptr )
		return false;

	tune.load ( filename );

	return finishLoad ();
}
//-----------------------------------------------------------------------------

bool Player::loadSidFile ( SidTune::LoaderFunc loader, const char* filename )
{
	readyToPlay = false;
	stiEZ = {};

	if ( sharedConfig == nullptr )
		return false;

	// Archive paths always use forward slashes
	tune.load ( loader, filename, true );

	return finishLoad ();
}
//-----------------------------------------------------------------------------

bool Player::finishLoad ()
{
	auto	info = tune.getInfo ();
	if ( ! info )
		return false;

	// Only the PSID loader computes an MD5; .prg/.p00 return null
	const auto	md5 = tune.createMD5New ();
	stiEZ.md5 = md5 ? md5 : "";

	tuneOverride = sharedConfig->overrideSelector.getOverride ( info->path (), info->dataFileName () );

	// Fill basic tune information (global for all songs)
	{
		stiEZ.title = stringutils::extendedASCIItoUTF8 ( info->infoString ( 0 ) );
		stiEZ.author = stringutils::extendedASCIItoUTF8 ( info->infoString ( 1 ) );
		stiEZ.released = stringutils::extendedASCIItoUTF8 ( info->infoString ( 2 ) );

		stiEZ.filename = std::string ( info->path () ) + std::string ( info->dataFileName () );

		stiEZ.numSongs = info->songs ();

		stiEZ.startSong = tuneOverride.startTune ? tuneOverride.startTune : info->startSong ();

		stiEZ.playroutineID = sharedConfig->sidID.findPlayerRoutines ( tune.getSidData () );

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

	if ( sharedConfig == nullptr )
		return false;

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
		stiEZ.model.clear ();

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
		const auto chipProfile = sharedConfig->chipSelector.getProfile ( info->path (), info->dataFileName (), stiEZ.currentSong );

		stiEZ.chipProfile = chipProfile.name;
		stiEZ.chipProfileIsApproved = chipProfile.isApproved;

		engine.set6581Filter_uCoxAndCap ( 20.0, chipProfile.fltCapOld );
		engine.set6581FilterCurve ( chipProfile.flt0Dac );
		engine.set6581FilterGain ( chipProfile.fltGain );
		engine.set6581FilterSaturation ( chipProfile.fltSaturation );
		engine.set6581FilterBandpassWidthOffset ( chipProfile.fltBandpassWidthOffset );

		engine.set6581DigiVolume ( chipProfile.digi );

		engine.set6581LeakageRate ( chipProfile.leakageRate );

		engine.setCombinedWaveforms ( reSIDfp::CombinedWaveforms ( chipProfile.cwsLevel ), 1.0f );
		engine.set6581SawPulseUltra ( chipProfile.cwsSawPulseUltra );

		stiEZ.chipSettingsValues = describeAppliedSettings ( chipProfile );
	}

	// Override chip-profile for Emulation based SID editors (Cheesecutter, GoatTracker, SidWizard etc.)
	{
		if ( ! stiEZ.playroutineID.empty () )
		{
			struct EmuEditors
			{
				std::string	id;
				std::string	name;
			};

			static const std::vector<EmuEditors> editorsUsingEmulation = {
				{ "CheeseCutter_",      "CheeseCutter"	},
				{ "GoatTracker_V",      "GoatTracker"	},
				{ "SidWizard_",         "SidWizard"		},
				{ "Hermit/SidWizard_V", "SidWizard"		},
				{ "SidFactory/",        "SidFactory"	},
				{ "SidFactory_II/",     "SidFactory II"	},
				{ "DefleMask_",         "DefleMask"		},
			};

			auto oldEmulation = [ this ] ( const EmuEditors& ed )
			{
				stiEZ.chipProfile = "emu-" + ed.name;

				engine.set6581Filter_uCoxAndCap ( 20.0, false );
				engine.set6581FilterCurve ( 0.5 );
				engine.set6581FilterGain ( 1.0 );
				engine.set6581FilterSaturation ( 1.0 );
				engine.set6581FilterBandpassWidthOffset ( 0.0 );
				engine.set6581DigiVolume ( 1.0 );
				engine.set6581LeakageRate ( 1.0 );

				engine.setCombinedWaveforms ( reSIDfp::CombinedWaveforms::AVERAGE, 1.0 );
				engine.set6581SawPulseUltra ( false );

				// Mirrors the fixed values above
				ChipProfileSelector::settings	neutral;
				neutral.flt0Dac = 0.5;
				neutral.fltGain = 1.0;
				stiEZ.chipSettingsValues = describeAppliedSettings ( neutral );
			};

			for ( const auto& id : editorsUsingEmulation )
				if ( stiEZ.playroutineID[ 0 ].starts_with ( id.id ) )
					oldEmulation ( id );
		}
	}

	// Dedicated sample players and the playback technique each one uses; the
	// mode implies the register its samples ride on
	{
		using DigiMode = reSIDfp::DigiMode;

		struct DigiPlayer
		{
			std::string_view	id;
			DigiMode			mode;
		};

		static constexpr DigiPlayer digiPlayers[] = {
			{ "8bitDigi/Mahoney",   DigiMode::mahoney },
			{ "OxyMod4Bit/THCM",    DigiMode::mahoney },
			{ "AnnoyedArt1256_Digi", DigiMode::mahoney },
			{ "OxyMod/THCM",        DigiMode::freq3 },
			{ "Algorithm/8bitDigi", DigiMode::freq3 },
			{ "Censor_8bit_Digi_1", DigiMode::freq3 },
			{ "Abaddon_Digi",       DigiMode::freq3 },
			{ "Groepaz_8bit_Digi",  DigiMode::freq2 },
			{ "Censor_8bit_Digi_2", DigiMode::voice3Out },
			{ "Censor_Digi_2",      DigiMode::voice1Pwm },
			{ "Cyberbrain_Digi",    DigiMode::pwHi1 },
			{ "Censor_Digi/16khz",  DigiMode::pwLo1 },
			{ "Silas_Warner_Digi",  DigiMode::filt1 },
			{ "StreetTuff_Digi",    DigiMode::pwFull1 },
			{ "Voicemaster_Covox",  DigiMode::covox },
		};

		// One-off rips whose player has no sidid entry, keyed by tune md5
		struct DigiTune
		{
			std::string_view	md5;
			DigiMode			mode;
		};

		static constexpr DigiTune digiTunes[] = {
			{ "40e7840d61e508d4c9be68a2b848b35b", DigiMode::freq1 },	// Vicious_SID_2-15638Hz
			{ "c5e7d1a5ce3e8bd886fdb04801f96adc", DigiMode::carmina },	// Vicious_SID_2-Carmina_Burana
			{ "48a3418ebaa3faf66e58dd9644503e4a", DigiMode::escos },	// Vicious_SID_2-Escos
			{ "48bcfe1e849627dd680a7b8d69a9d432", DigiMode::escos },	// Vicious_SID_2-1st_loader, same impulse school
			{ "36866f276dbfa35d3d407ba7ad3cd01e", DigiMode::output },	// FRODIGI
			{ "1e662d0b7cb80ecb686877471a9a47cc", DigiMode::output },	// FRODIGI_2
			{ "ccfa92d4441dcc6e84524a0826126844", DigiMode::output },	// FRODIGI_3
			{ "c5183a95ffeffc5fbe30f190c685bd77", DigiMode::output },	// FRODIGI_4_tune_1
			{ "ddf01a31a0bc52fe9d5300f9e7afeb67", DigiMode::output },	// FRODIGI_4_tune_2
			{ "17b06cb2477d3d8b24e5b14a8709df29", DigiMode::covox },		// Out_on_a_Limb, speech in all subtunes but the first
			{ "e771c1358d9143864e6fea46a1c9a8b8", DigiMode::voice1Pwm },	// Wonderland_IX_part_5, PWM samples on voices 1+2
			{ "d29612059e480333104edf8315fef76f", DigiMode::voice1Pwm },	// Spasmolytic_part_2, PWM samples on voice 1
			{ "74323308a10bbb02d713a23c6b5b16e7", DigiMode::covox },		// All_Risks, speech in subtune 4
			{ "b8b3fe215c067d3e4bf0a4cee8eed753", DigiMode::pwLo1 },		// Wonderland_XI-End, 16 kHz Censor school without a signature
		};

		stiEZ.digiMode = DigiMode::nibble;
		stiEZ.digiPlayer = false;

		for ( const auto& player : digiPlayers )
			if ( std::ranges::contains ( stiEZ.playroutineID, player.id ) )
			{
				stiEZ.digiMode = player.mode;
				stiEZ.digiPlayer = true;
				break;
			}

		if ( ! stiEZ.digiPlayer )
			for ( const auto& digiTune : digiTunes )
				if ( stiEZ.md5 == digiTune.md5 )
				{
					stiEZ.digiMode = digiTune.mode;
					stiEZ.digiPlayer = true;
					break;
				}

		engine.setDigiCapture ( stiEZ.digiMode );
	}

	//
	// Get audio profile for specific 2SID and 3SID, and even some 1SID tunes. Most will be mixed to mono,
	// but we can provide a list where we want a full or narrowed stereo field and bass-adjustment
	//
	{
		const auto audioProfile = sharedConfig->audioSelector.getProfile ( info->path (), info->dataFileName () );

		// The mixer follows the same flag when placing the chips
		stiEZ.wantsStereo = info->hasSidChannels ();

		if ( audioProfile )
		{
			stiEZ.stereoWidth = audioProfile->width;
			stiEZ.bassAdjust = float ( audioProfile->bass );
		}
		else if ( stiEZ.wantsStereo )
		{
			// A tune that places its chips gets the full authored field
			stiEZ.stereoWidth = 100;
		}
	}

	return readyToPlay;
}
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
