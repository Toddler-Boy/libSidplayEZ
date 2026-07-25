#pragma once

#include <string>
#include <unordered_map>

namespace libsidplayEZ
{
//-----------------------------------------------------------------------------

class AudioProfileSelector final
{
public:
	struct settings final
	{
		// Stereo width (in percent)
		int		width = 0;

		// Bass adjustment (usually negative values to push them down, as it's usually boosted by 6-9dB)
		double	bass = 0.0;
	};

	using profileMap = std::unordered_map<std::string, settings>;

	settings getProfile ( const char* path, const char* filename ) const;
	// Returns a description of the first unusable cell, empty when the file was clean
	std::string setProfiles ( const std::string& csvStr );

private:
	profileMap	stereoProfiles;
};
//-----------------------------------------------------------------------------

}
