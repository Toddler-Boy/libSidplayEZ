#include <algorithm>
#include <cmath>
#include <numbers>

#include "audio-profile-selector.h"

#include "tinyCSV.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

AudioProfileSelector::settings AudioProfileSelector::getProfile ( const char* _path, const char* _filename )
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

void AudioProfileSelector::setProfiles ( const std::string& csvStr )
{
	stereoProfiles.clear ();

	auto	csv = TinyCSV ();

	const auto	rows = csv.parseCSV ( csvStr );
	for ( auto i = 0; i < rows; ++i )
	{
		const auto	name = csv.get ( i, "tune", "" );

		settings	setting;

		setting.width = csv.get ( i, "width", setting.width );
		setting.bass = csv.get ( i, "bass", setting.bass );

		stereoProfiles[ name ] = setting;
	}
}
//-----------------------------------------------------------------------------

void AudioProfileSelector::downMix ( float* __restrict__ srcDstL, float* __restrict__ srcDstR, const int numSamples, const float width )
{
	// Pure stereo, no down mixing required
	if ( width >= ( 1.0f - 1e-6f ) )
		return;

	constexpr auto	quarterSine = 0.70710678118f;

	// Pure mono
	if ( width < 1e-6f )
	{
		for ( auto i = 0; i < numSamples; i++ )
		{
			const auto	mid = ( srcDstL[ i ] + srcDstR[ i ] ) * quarterSine;

			srcDstL[ i ] = mid;
			srcDstR[ i ] = mid;
		}
		return;
	}

	const auto	midGain = std::cos ( 0.25f * std::numbers::pi_v<float> * width ) * quarterSine;
	const auto	sideGain = std::sin ( 0.25f * std::numbers::pi_v<float> * width ) * quarterSine;

	// Down mix
	for ( auto i = 0; i < numSamples; i++ )
	{
		const auto	left = srcDstL[ i ];
		const auto	right = srcDstR[ i ];

		const auto	mid = ( left + right ) * midGain;
		const auto	side = ( left - right ) * sideGain;

		srcDstL[ i ] = mid + side;
		srcDstR[ i ] = mid - side;
	}
}
//-----------------------------------------------------------------------------

}
