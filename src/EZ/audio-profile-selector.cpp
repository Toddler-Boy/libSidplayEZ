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
	const auto	midGain = std::cos ( 0.25f * std::numbers::pi_v<float> *width );
	const auto	sideGain = std::sin ( 0.25f * std::numbers::pi_v<float> *width );

	constexpr auto isEqual = [] ( const float a, const float b )
	{
		constexpr auto	smallDelta = 1e-6f;
		return std::abs ( a - b ) < smallDelta;
	};

	// Pure stereo, no down mixing required
	if ( isEqual ( midGain, 0.0f ) && isEqual ( sideGain, 1.0f ) )
		return;

	// Pure mono
	if ( isEqual ( midGain, 1.0f ) && isEqual ( sideGain, 0.0f ) )
	{
		for ( auto i = 0; i < numSamples; i++ )
		{
			const auto	mid = srcDstL[ i ] + srcDstR[ i ];

			srcDstL[ i ] = mid;
			srcDstR[ i ] = mid;
		}
		return;
	}

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
