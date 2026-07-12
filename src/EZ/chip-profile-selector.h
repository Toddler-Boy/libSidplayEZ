#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace libsidplayEZ
{
//-----------------------------------------------------------------------------

class ChipProfileSelector final
{
public:
	enum : uint8_t
	{
		weak,
		average,
		strong,
	};

	struct settings final
	{
		std::string	name;
		std::string	folder;

		// Status (approved or best-guess)
		bool		isApproved = false;

		// Filter settings
		bool		fltCapOld = false;
		double		flt0Dac = 0.4;
		double		fltGain = 0.92;
		double		fltSaturation = 1.0;
		double		fltBandpassWidthOffset = 0.0;

		// Digi value
		double		digi = 1.0;

		// Combined waveform strength
		int			cwsLevel = average;
		bool		cwsSawPulseUltra = false;

		// Exceptions (selects another authors chip, usually for collaborations)
		std::unordered_map<std::string, std::string>	exceptions;
	};

	using profileMap = std::unordered_map<std::string, settings>;

	settings getProfile ( const char* path, const char* filename, const int subtune );

	void setProfiles ( const std::string& csvStr );

private:
	profileMap	chipProfiles;
};
//-----------------------------------------------------------------------------

}
