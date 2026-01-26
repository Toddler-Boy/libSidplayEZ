#pragma once
/*
* This file is part of libsidplayfp, a SID player engine.
*
*  Copyright 2011-2015 Leandro Nini
*  Copyright 2007-2010 Antti Lankila
*  Copyright 2000 Simon White
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <cstdint>
#include <vector>
#include <string>

#include "../sidplayfp/SidTuneInfo.h"

namespace libsidplayfp
{

/**
* The implementation of the SidTuneInfo interface.
*/
class SidTuneInfoImpl final : public SidTuneInfo
{
public:
	std::string	m_formatString = "N/A";

	unsigned int m_songs = 0;
	unsigned int m_startSong = 0;
	unsigned int m_currentSong = 0;

	int	m_songSpeed = SPEED_VBI;

	clock_t			m_clockSpeed = CLOCK_UNKNOWN;
	compatibility_t	m_compatibility = COMPATIBILITY_C64;

	uint32_t m_dataFileLen = 0;
	uint32_t m_c64dataLen = 0;

	uint16_t m_loadAddr = 0;
	uint16_t m_initAddr = 0;
	uint16_t m_playAddr = 0;

	uint8_t m_relocStartPage = 0;
	uint8_t m_relocPages = 0;

	std::string	m_path;
	std::string	m_dataFileName;
	std::string	m_infoFileName;

	std::vector<model_t>	m_sidModels = { SIDMODEL_UNKNOWN };
	std::vector<uint16_t>	m_sidChipAddresses = { 0xD400 };

	std::vector<std::string>	m_infoString;
	std::vector<std::string>	m_commentString;

	bool	m_fixLoad = false;

private:
	// prevent copying
	SidTuneInfoImpl ( const SidTuneInfoImpl& ) = delete;
	SidTuneInfoImpl& operator=( SidTuneInfoImpl& ) = delete;

public:
	SidTuneInfoImpl () = default;

	[[ nodiscard ]] uint16_t getLoadAddr () const override { return m_loadAddr; }
	[[ nodiscard ]] uint16_t getInitAddr () const override { return m_initAddr; }
	[[ nodiscard ]] uint16_t getPlayAddr () const override { return m_playAddr; }

	[[ nodiscard ]] unsigned int getSongs () const override { return m_songs; }
	[[ nodiscard ]] unsigned int getStartSong () const override { return m_startSong; }
	[[ nodiscard ]] unsigned int getCurrentSong () const override { return m_currentSong; }

	[[ nodiscard ]] uint16_t getSidChipBase ( unsigned int i ) const override	{	return i < m_sidChipAddresses.size () ? m_sidChipAddresses[ i ] : 0;	}

	[[ nodiscard ]] int getSidChips () const override { return int ( m_sidChipAddresses.size () ); }

	[[ nodiscard ]] int getSongSpeed () const override { return m_songSpeed; }

	[[ nodiscard ]] uint8_t getRelocStartPage () const override { return m_relocStartPage; }
	[[ nodiscard ]] uint8_t getRelocPages () const override { return m_relocPages; }

	[[ nodiscard ]] model_t getSidModel ( unsigned int i ) const override	{	return i < m_sidModels.size () ? m_sidModels[ i ] : SIDMODEL_UNKNOWN;	}

	[[ nodiscard ]] compatibility_t getCompatibility () const override { return m_compatibility; }

	[[ nodiscard ]] unsigned int getNumberOfInfoStrings () const override { return (unsigned int)m_infoString.size (); }
	[[ nodiscard ]] const char* getInfoString ( unsigned int i ) const override { return i < getNumberOfInfoStrings () ? m_infoString[ i ].c_str () : ""; }

	[[ nodiscard ]] unsigned int getNumberOfCommentStrings () const override { return (unsigned int)m_commentString.size (); }
	[[ nodiscard ]] const char* getCommentString ( unsigned int i ) const override { return i < getNumberOfCommentStrings () ? m_commentString[ i ].c_str () : ""; }

	[[ nodiscard ]] uint32_t getDataFileLen () const override { return m_dataFileLen; }
	[[ nodiscard ]] uint32_t getC64dataLen () const override { return m_c64dataLen; }
	[[ nodiscard ]] clock_t getClockSpeed () const override { return m_clockSpeed; }

	[[ nodiscard ]] const char* getFormatString () const override { return m_formatString.c_str (); }

	[[ nodiscard ]] bool getFixLoad () const override { return m_fixLoad; }

	[[ nodiscard ]] const char* getPath () const override { return m_path.c_str (); }
	[[ nodiscard ]] const char* getDataFileName () const override { return m_dataFileName.c_str (); }
	[[ nodiscard ]] const char* getInfoFileName () const override { return ! m_infoFileName.empty () ? m_infoFileName.c_str () : nullptr; }
};

}
