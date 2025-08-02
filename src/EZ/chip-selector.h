#pragma once

#include <unordered_map>
#include <string>

namespace libsidplayEZ
{
//-----------------------------------------------------------------------------

class ChipSelector final
{
public:
	ChipSelector ();

	enum
	{
		weak,
		average,
		strong,
	};

	struct settings final
	{
		std::string	folder;

		// Filter settings
		double		fltCox = 0.5;
		double		flt0Dac = 0.4;
		double		fltGain = 0.92;

		// Digi value
		double		digi = 1.0;

		// Combined waveform strength
		int			cwsLevel = average;
		double		cwsThreshold = 1.0;

		std::unordered_map<std::string, std::string>	exceptions;
	};

	using profileMap = std::unordered_map<std::string, settings>;

	std::pair<std::string, settings> getChipProfile ( const char* path, const char* filename );

	void setProfiles ( const std::string& csvStr );

private:
	profileMap	chipProfiles;
};
//-----------------------------------------------------------------------------

}
