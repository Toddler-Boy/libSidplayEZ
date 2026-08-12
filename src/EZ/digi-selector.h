#pragma once

#include <string>
#include <vector>

#include "../sidplayfp/residfp/DigiMode.h"

namespace libsidplayEZ
{
//-----------------------------------------------------------------------------

class DigiSelector final
{
public:
	struct digiInfo final
	{
		reSIDfp::DigiMode	mode = reSIDfp::DigiMode::nibble;
		bool				digiPlayer = false;

		// An entry exists either way; "none" rows are covered non-digis
		bool				covered = false;
	};

	// The tune entry wins over any player signature, being the most specific
	digiInfo getDigi ( const char* path, const char* filename, const std::vector<std::string>& playroutineIDs ) const;

	// Each returns a description of the first unusable cell, empty when the file was clean
	std::string setDigiPlayer ( const std::string& csvStr );
	std::string setDigiTunes ( const std::string& csvStr );

private:
	struct playerEntry final
	{
		std::string			id;
		reSIDfp::DigiMode	mode;
	};

	struct tuneEntry final
	{
		std::string			tune;
		reSIDfp::DigiMode	mode;
		bool				digi;	// false = a "none" row, an established non-digi
	};

	std::vector<playerEntry>	digiPlayers;
	std::vector<tuneEntry>		digiTunes;
};
//-----------------------------------------------------------------------------

}
