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
		setting.fltCapOld = stringutils::toLower ( csv.get ( i, "fltCap" ) ) == "old";
		setting.flt0Dac = csv.get ( i, "flt0Dac", setting.flt0Dac );
		setting.fltGain = csv.get ( i, "fltGain", setting.fltGain );
		setting.digi = csv.get ( i, "digi", setting.digi );

		// Combined waveform strength level
		auto	cwsLevel = stringutils::toLower ( csv.get ( i, "cwsLevel", "average" ) );

		// Check for ultra sawPulse setting (indicated by a '+' at the end of the cwsLevel)
		setting.cwsSawPulseUltra = cwsLevel.back () == '+';
		if ( setting.cwsSawPulseUltra )
			cwsLevel.erase ( cwsLevel.length () - 1 );

		if ( cwsLevel == "weak" )			setting.cwsLevel = weak;
		else if ( cwsLevel == "strong" )	setting.cwsLevel = strong;

		// Exceptions
		if ( const auto	exceptions = csv.get ( i, "exceptions" ); ! exceptions.empty () )
		{
			auto	exceptionList = stringutils::arrayFromTokens ( exceptions, ';' );
			for ( const auto& exception : exceptionList )
			{
				if ( const auto file_profile = stringutils::arrayFromTokens ( exception, '=' ); file_profile.size () == 2 )
				{
					if ( const auto file_ranges = stringutils::arrayFromTokens ( file_profile[ 0 ], '#' ); file_ranges.size () == 2 )
					{
						if ( const auto ranges = stringutils::arrayFromTokens ( file_ranges[ 1 ], ',' ); ! ranges.empty () )
						{
							for ( const auto& range : ranges )
							{
								const auto	subtune = stringutils::arrayFromTokens ( range, '-' );
								const auto	subtuneStart = std::stoi ( subtune[ 0 ] );
								const auto	subtuneEnd = subtune.size () == 2 ? std::stoi ( subtune[ 1 ] ) : subtuneStart;

								for ( auto st = subtuneStart; st <= subtuneEnd; ++st )
									setting.exceptions[ file_ranges[ 0 ] + "#" + std::to_string ( st ) ] = file_profile[ 1 ];
							}
						}
					}
				}
			}
		}

		chipProfiles[ name ] = setting;
	}
}
//-----------------------------------------------------------------------------

}
