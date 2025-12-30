#include "override-selector.h"

#include "tinyCSV.h"

#include <algorithm>
#include "../stringutils.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

constexpr char defaultOverrides[] = {
#embed "tune-overrides.csv"
};

OverrideSelector::OverrideSelector ()
{
	setOverrides ( std::string ( defaultOverrides, sizeof ( defaultOverrides ) ) );
}
//-----------------------------------------------------------------------------

OverrideSelector::overrides OverrideSelector::getOverride ( const char* _path, const char* _filename )
{
	auto	path = std::string ( _path ) + std::string ( _filename );

	// Normalize path separators
	std::replace ( path.begin (), path.end (), '\\', '/' );

	// Remove root
	auto	pos = path.rfind ( "/MUSICIANS/" );
	if ( pos == std::string::npos )		pos = path.rfind ( "/DEMOS/" );
	if ( pos == std::string::npos )		pos = path.rfind ( "/GAMES/" );
	if ( pos == std::string::npos )
		return {};	// No profile found, use default

	path = path.substr ( pos );

	const auto& it = std::find_if ( tuneOverrides.begin (), tuneOverrides.end (), [ &path ] ( const auto& a )
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

		setting.tune = csv.getString ( i, "tune" );

		setting.startTune = uint16_t ( csv.getInt ( i, "start", 0 ) );

		const auto	chip = csv.getString ( i, "chip", "" );
		if ( chip == "6581" )
			setting.chipModel = 1u;
		else if ( chip == "8580" )
			setting.chipModel = 2u;

		const auto	clock = stringutils::toLower ( csv.getString ( i, "clock", "" ) );
		if ( clock == "pal" )
			setting.clock = 1u;
		else if ( clock == "ntsc" )
			setting.clock = 2u;

		tuneOverrides.emplace_back ( setting );
	}
}
//-----------------------------------------------------------------------------
}
