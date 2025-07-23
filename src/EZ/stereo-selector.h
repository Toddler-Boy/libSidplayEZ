#pragma once

#include <unordered_map>
#include <string>

namespace libsidplayEZ
{
//-----------------------------------------------------------------------------

class StereoSelector final
{
public:
	enum
	{
		mono = 0,
		narrow = 20,
		full = 100,
	};

	struct settings final
	{
		// Stereo width (in percent)
		int		width = mono;

		// Bass adjustment (usually negative values to push them down, as it's usually boosted by 6-9dB)
		double	bass = 0.0;
	};

	using profileMap = std::unordered_map<std::string, settings>;

	settings getStereoProfile ( const char* path, const char* filename );
	void setProfiles ( const profileMap& map );

private:
	profileMap	stereoProfiles = {
		#include "stereo-profiles.h"
	};
};
//-----------------------------------------------------------------------------

}
