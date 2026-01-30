#include <algorithm>

#include "override-selector.h"
#include "tinyCSV.h"

#include "../stringutils.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

OverrideSelector::overrides OverrideSelector::getOverride ( const char* _path, const char* _filename )
{
	auto	path = std::string ( _path ) + std::string ( _filename );

	// Normalize path separators
	std::ranges::replace ( path, '\\', '/' );

	// Remove root
	auto	pos = path.rfind ( "/MUSICIANS/" );
	if ( pos == std::string::npos )		pos = path.rfind ( "/DEMOS/" );
	if ( pos == std::string::npos )		pos = path.rfind ( "/GAMES/" );
	if ( pos == std::string::npos )
		return {};	// No profile found, use default

	path = path.substr ( pos );

	const auto& it = std::ranges::find_if ( tuneOverrides, [ &path ] ( const auto& a )
	{
		return path.starts_with ( a.tune );
	} );

	if ( it != tuneOverrides.end () )
		return *it;

	return {};
}
//-----------------------------------------------------------------------------

void OverrideSelector::setOverrides ( const std::string& csvStr )
{
	tuneOverrides.clear ();

	auto	csv = TinyCSV ();

	const auto	rows = csv.parseCSV ( csvStr );
	for ( auto i = 0; i < rows; ++i )
	{
		overrides	setting;

		setting.tune = csv.get ( i, "tune" );

		setting.startTune = uint16_t ( csv.get ( i, "start", 0 ) );

		const auto	chip = csv.get ( i, "chip" );
		if ( chip == "6581" )		setting.chipModel = 1u;
		else if ( chip == "8580" )	setting.chipModel = 2u;

		const auto	clock = stringutils::toUpper ( csv.get ( i, "clock" ) );
		if ( clock == "PAL" )		setting.clock = 1u;
		else if ( clock == "NTSC" )	setting.clock = 2u;

		tuneOverrides.emplace_back ( setting );
	}
}
//-----------------------------------------------------------------------------
}
