#pragma once

#include <vector>
#include <string>

namespace libsidplayEZ
{
//-----------------------------------------------------------------------------

class OverrideSelector final
{
public:
	struct overrides final
	{
		std::string	tune;
		uint16_t	startTune = 0;	// 0 = from tune
		uint8_t		chipModel = 0;	// 0 = from tune, 1 = MOS6581, 2 = MOS8580
		uint8_t		clock = 0;		// 0 = from tune, 1 = PAL, 2 = NTSC (only overrides when unknown)
	};

	using overrideMap = std::vector<overrides>;

	overrides getOverride ( const char* path, const char* filename );
	void setOverrides ( const std::string& csvStr );

	const overrideMap& getAllOverrides () const { return tuneOverrides; }

private:
	overrideMap	tuneOverrides;
};
//-----------------------------------------------------------------------------

}
