#include "stereo-selector.h"

#include "tinyCSV.h"
#include "../stringutils.h"

#include <algorithm>

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

constexpr char defaultProfiles[] = {
#embed "stereo-profiles.csv"
};

StereoSelector::StereoSelector ()
{
	setProfiles ( defaultProfiles );
}
//-----------------------------------------------------------------------------

StereoSelector::settings StereoSelector::getStereoProfile ( const char* _path, const char* _filename )
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

	// Try to find direct match
	{
		const auto&	it = stereoProfiles.find ( path );
		if ( it != stereoProfiles.end () )
			return it->second;
	}

	// Try to find wildcard match
	{
		const auto	underscore = path.find_last_of ( '_' );
		const auto	slash = path.find_last_of ( '/' );

		if ( slash > underscore || underscore == std::string::npos )
			return {};

		const auto	wildcard = path.substr ( 0, slash + 1 ) + "*" + path.substr ( underscore );

		const auto&	it = stereoProfiles.find ( wildcard );
		if ( it != stereoProfiles.end () )
			return it->second;
	}

	return {};
}
//-----------------------------------------------------------------------------

void StereoSelector::setProfiles ( const std::string& csvStr )
{
	auto	csv = TinyCSV ();

	const auto	rows = csv.parseCSV ( csvStr );
	for ( auto i = 0; i < rows; ++i )
	{
		const auto	name = csv.getString ( i, "tune" );

		settings	setting;

		setting.width = csv.getInt ( i, "width", 0 );
		setting.bass = csv.getDouble ( i, "bass", setting.bass );

		stereoProfiles[ name ] = setting;
	}
}
//-----------------------------------------------------------------------------

}
