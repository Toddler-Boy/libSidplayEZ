#include "stereo-selector.h"

#include <algorithm>

namespace libsidplayEZ
{

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
		const auto	wildcard = path.substr ( 0, path.find_last_of ( '/' ) + 1 ) + "*" + path.substr ( path.find_last_of ( '_' ) );

		const auto&	it = stereoProfiles.find ( wildcard );
		if ( it != stereoProfiles.end () )
			return it->second;
	}

	return {};
}
//-----------------------------------------------------------------------------

void StereoSelector::setProfiles ( const profileMap& map )
{
	stereoProfiles = map;
}
//-----------------------------------------------------------------------------

}
