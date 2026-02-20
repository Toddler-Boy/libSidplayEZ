#include <algorithm>

#include "chip-profile-selector.h"
#include "tinyCSV.h"
#include "../stringutils.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

std::pair<std::string, ChipProfileSelector::settings> ChipProfileSelector::getProfile ( const char* _path, const char* _filename, const int subtune )
{
	auto	path = std::string ( _path );

	// Normalize path separators
	std::ranges::replace ( path, '\\', '/' );

	// Remove root
	auto	pos = path.rfind ( "/MUSICIANS/" );

	// If tune is not from a "/MUSICIANS/" folder, return default values
	if ( pos == std::string::npos )
		return {};

	path = path.substr ( pos );

	std::string	bestPath;
	std::string	bestProfile;

	// Identify author by folder (longest matching path wins)
	for ( const auto& [ name, set ] : chipProfiles )
	{
		if ( ! path.starts_with ( set.folder ) )
			continue;

		if ( set.folder.size () < bestPath.size () )
			continue;

		bestPath = set.folder;
		bestProfile = name;
	}

	// No profile found, return defaults
	if ( bestProfile.empty () )
		return {};

	// Get author profile
	const auto&	set = chipProfiles.at ( bestProfile );

	// No exceptions, return profile
	if ( set.exceptions.empty () )
		return std::make_pair ( bestProfile, set );

	// Get filename without extension
	auto	filename = std::string ( _filename );
	filename.erase ( filename.length () - 4 );

	// Attach tune-number to filename
	filename += "#" + std::to_string ( subtune );

	// Find new author if exception matches
	if ( auto exception = set.exceptions.find ( filename ); exception != set.exceptions.end () )
		return std::make_pair ( exception->second, chipProfiles.at ( exception->second ) );

	// No exception matched, return best profile
	return std::make_pair ( bestProfile, set );
}
//-----------------------------------------------------------------------------

void ChipProfileSelector::setProfiles ( const std::string& csvStr )
{
	chipProfiles.clear ();

	auto	csv = TinyCSV ();

	const auto	rows = csv.parseCSV ( csvStr );
	for ( auto i = 0; i < rows; ++i )
	{
		const auto	name = csv.get ( i, "name" );

		settings	setting;

		setting.folder = csv.get ( i, "folder" );
		setting.fltCox = csv.get ( i, "fltCox", setting.fltCox );
		setting.flt0Dac = csv.get ( i, "flt0Dac", setting.flt0Dac );
		setting.fltGain = csv.get ( i, "fltGain", setting.fltGain );
		setting.digi = csv.get ( i, "digi", setting.digi );

		const auto	cwsLevel = stringutils::toLower ( csv.get ( i, "cwsLevel", "average" ) );

		if ( cwsLevel == "weak" )			setting.cwsLevel = weak;
		else if ( cwsLevel == "strong" )	setting.cwsLevel = strong;

		const auto	exceptions = csv.get ( i, "exceptions" );
		if ( ! exceptions.empty () )
		{
			auto	exceptionList = stringutils::arrayFromTokens ( exceptions, ';' );
			for ( const auto& exception : exceptionList )
				if ( auto pair = stringutils::arrayFromTokens ( exception, '=' ); pair.size () == 2 )
					setting.exceptions[ pair[ 0 ] ] = pair[ 1 ];
		}

		chipProfiles[ name ] = setting;
	}
}
//-----------------------------------------------------------------------------

}
