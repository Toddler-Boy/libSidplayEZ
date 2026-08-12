#include <algorithm>
#include <array>
#include <utility>

#include "digi-selector.h"
#include "tinyCSV.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

using DigiMode = reSIDfp::DigiMode;

// The CSV vocabulary; the raw* measurement modes are internal and stay out
static constexpr std::array<std::pair<std::string_view, DigiMode>, 18>	modeNames = { {
	{ "nibble",		DigiMode::nibble },
	{ "mahoney",	DigiMode::mahoney },
	{ "freq1",		DigiMode::freq1 },
	{ "freq2",		DigiMode::freq2 },
	{ "freq3",		DigiMode::freq3 },
	{ "pwLo1",		DigiMode::pwLo1 },
	{ "pwHi1",		DigiMode::pwHi1 },
	{ "pwFull1",	DigiMode::pwFull1 },
	{ "filt1",		DigiMode::filt1 },
	{ "voice3Out",	DigiMode::voice3Out },
	{ "voice1Pwm",	DigiMode::voice1Pwm },
	{ "covox",		DigiMode::covox },
	{ "carmina",	DigiMode::carmina },
	{ "escos",		DigiMode::escos },
	{ "output",		DigiMode::output },
	{ "output2x",	DigiMode::output2x },
	{ "output3x",	DigiMode::output3x },
	{ "output4x",	DigiMode::output4x },
} };

static const DigiMode* findMode ( const std::string& name )
{
	for ( const auto& [ modeName, mode ] : modeNames )
		if ( name == modeName )
			return &mode;

	return nullptr;
}
//-----------------------------------------------------------------------------

DigiSelector::digiInfo DigiSelector::getDigi ( const char* _path, const char* _filename, const std::vector<std::string>& playroutineIDs ) const
{
	auto	path = std::string ( _path ) + std::string ( _filename );

	// Normalize path separators
	std::ranges::replace ( path, '\\', '/' );

	// Remove root
	auto	pos = path.rfind ( "/MUSICIANS/" );
	if ( pos == std::string::npos )		pos = path.rfind ( "/DEMOS/" );
	if ( pos == std::string::npos )		pos = path.rfind ( "/GAMES/" );

	if ( pos != std::string::npos )
	{
		path = path.substr ( pos );

		for ( const auto& tune : digiTunes )
			if ( path == tune.tune )
				return { tune.mode, tune.digi, true };
	}

	for ( const auto& player : digiPlayers )
		if ( std::ranges::contains ( playroutineIDs, player.id ) )
			return { player.mode, true, true };

	return {};
}
//-----------------------------------------------------------------------------

std::string DigiSelector::setDigiPlayer ( const std::string& csvStr )
{
	digiPlayers.clear ();

	auto	csv = TinyCSV ();

	const auto	rows = csv.parseCSV ( csvStr );
	for ( auto i = 0; i < rows; ++i )
	{
		const auto	id = csv.get ( i, "player" );
		const auto	modeName = csv.get ( i, "mode" );

		const auto	mode = findMode ( modeName );
		if ( ! mode )
			return "player '" + id + "': '" + modeName + "' is not a digi mode";

		digiPlayers.emplace_back ( id, *mode );
	}

	return csv.getError ();
}
//-----------------------------------------------------------------------------

std::string DigiSelector::setDigiTunes ( const std::string& csvStr )
{
	digiTunes.clear ();

	auto	csv = TinyCSV ();

	const auto	rows = csv.parseCSV ( csvStr );
	for ( auto i = 0; i < rows; ++i )
	{
		const auto	tune = csv.get ( i, "tune" );
		const auto	modeName = csv.get ( i, "mode" );

		// "none" = an established non-digi: the verdict shields the tune
		// from the scanner's unknown-mode detection
		if ( modeName == "none" )
		{
			digiTunes.emplace_back ( tune, DigiMode::nibble, false );
			continue;
		}

		const auto	mode = findMode ( modeName );
		if ( ! mode )
			return "tune '" + tune + "': '" + modeName + "' is not a digi mode";

		digiTunes.emplace_back ( tune, *mode, true );
	}

	return csv.getError ();
}
//-----------------------------------------------------------------------------
}
