#include "chip-selector.h"

#include "tinyCSV.h"
#include "../stringutils.h"

#include <algorithm>

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

constexpr char defaultProfiles[] = {
#embed "chip-profiles.csv"
};

ChipSelector::ChipSelector ()
{
	setProfiles ( std::string ( defaultProfiles, sizeof ( defaultProfiles ) ) );
}
//-----------------------------------------------------------------------------

std::pair<std::string, ChipSelector::settings> ChipSelector::getChipProfile ( const char* _path, const char* _filename )
{
	auto	path = std::string ( _path );

	// Normalize path separators
	std::replace ( path.begin (), path.end (), '\\', '/' );

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

	// Find new author if exception matches
	if ( auto exception = set.exceptions.find ( filename ); exception != set.exceptions.end () )
		return std::make_pair ( exception->second, chipProfiles.at ( exception->second ) );

	// No exception matched, return best profile
	return std::make_pair ( bestProfile, set );
}
//-----------------------------------------------------------------------------

void ChipSelector::setProfiles ( const std::string& csvStr )
{
	auto	csv = TinyCSV ();

	const auto	rows = csv.parseCSV ( csvStr );
	for ( auto i = 0; i < rows; ++i )
	{
		const auto	name = csv.getString ( i, "name" );

		settings	setting;

		setting.folder = csv.getString ( i, "folder" );
		setting.fltCox = csv.getDouble ( i, "fltCox", setting.fltCox );
		setting.flt0Dac = csv.getDouble ( i, "flt0Dac", setting.flt0Dac );
		setting.fltGain = csv.getDouble ( i, "fltGain", setting.fltGain );
		setting.digi = csv.getDouble ( i, "digi", setting.digi );

		static const std::unordered_map<std::string, int> cwsLevels = {
			{ "weak", ChipSelector::weak },
			{ "average", ChipSelector::average },
			{ "strong", ChipSelector::strong }
		};

		setting.cwsLevel = cwsLevels.at ( stringutils::toLower ( csv.getString ( i, "cwsLevel", "average" ) ) );

		const auto	exceptions = csv.getString ( i, "exceptions" );
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
