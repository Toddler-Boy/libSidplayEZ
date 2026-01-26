#pragma once
/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cstdint>
#include <vector>

class SidTuneInfo;

namespace libsidplayfp
{
	class SidTuneBase;
	class sidmemory;
}

/**
 * SidTune
 */
class SidTune final
{
public:
	static constexpr int MD5_LENGTH = 32;

private:  // -------------------------------------------------------------
	libsidplayfp::SidTuneBase*	tune = nullptr;

	const char*	m_statusString = nullptr;

	bool	m_status;

public:  // ----------------------------------------------------------------
	typedef void ( *LoaderFunc )( const char* fileName, std::vector<uint8_t>& bufferRef );

	/**
	 * Load a sidtune from a file.
	 *
	 * You can specify "fileName = 0", if you do not want to load a sidtune. You can later load one with open().
	 *
	 * @param fileName
	 * @param separatorIsSlash
	 */
	SidTune ( const char* fileName = "", bool separatorIsSlash = false);

	/**
	 * Load a sidtune from a file, using a file access callback.
	 *
	 * This function does the same as the above, except that it
	 * accepts a callback function, which will be used to read
	 * all files it accesses.
	 *
	 * @param loader
	 * @param fileName
	 * @param separatorIsSlash
	 */
	SidTune ( LoaderFunc loader, const char* fileName = "", bool separatorIsSlash = false);

	/**
	 * Load a single-file sidtune from a memory buffer.
	 * Currently supported: PSID and MUS formats.
	 *
	 * @param oneFileFormatSidtune the buffer that contains song data
	 * @param sidtuneLength length of the buffer
	 */
	SidTune ( const uint8_t* oneFileFormatSidtune, uint32_t sidtuneLength );
	~SidTune ();

	/**
	 * Load a sidtune into an existing object from a file.
	 *
	 * @param fileName
	 * @param separatorIsSlash
	 */
	void load ( const char* fileName, bool separatorIsSlash = false );

	/**
	 * Load a sidtune into an existing object from a file,
	 * using a file access callback.
	 *
	 * @param loader
	 * @param fileName
	 * @param separatorIsSlash
	 */
	void load ( LoaderFunc loader, const char* fileName, bool separatorIsSlash = false );

	/**
	 * Load a sidtune into an existing object from a buffer.
	 *
	 * @param sourceBuffer the buffer that contains song data
	 * @param bufferLen length of the buffer
	 */
	void read ( const uint8_t* sourceBuffer, uint32_t bufferLen );

	/**
	 * Select sub-song.
	 *
	 * @param songNum the selected song (0 = default starting song)
	 * @return active song number, 0 if no tune is loaded.
	 */
	unsigned int selectSong ( unsigned int songNum );

	/**
	 * Retrieve current active sub-song specific information.
	 *
	 * @return a pointer to #SidTuneInfo, 0 if no tune is loaded. The pointer must not be deleted.
	 */
	[[ nodiscard ]] const SidTuneInfo* getInfo () const;

	/**
	 * Select sub-song and retrieve information.
	 *
	 * @param songNum the selected song (0 = default starting song)
	 * @return a pointer to #SidTuneInfo, 0 if no tune is loaded. The pointer must not be deleted.
	 */
	[[ nodiscard ]] const SidTuneInfo* getInfo ( unsigned int songNum );

	/**
	 * Determine current state of object.
	 * Upon error condition use #statusString to get a descriptive
	 * text string.
	 *
	 * @return current state (true = okay, false = error)
	 */
	[[ nodiscard ]] bool getStatus () const	{	return m_status;	}

	/**
	 * Error/status message of last operation.
	 */
	[[ nodiscard ]] const char* statusString () const { return m_statusString; }

	/**
	 * Copy sidtune into C64 memory (64 KB).
	 */
	bool placeSidTuneInC64mem ( libsidplayfp::sidmemory& mem );

	/**
	 * Calculates the MD5 hash of the tune, old method.
	 * Not providing an md5 buffer will cause the internal one to be used.
	 * If provided, buffer must be MD5_LENGTH + 1
	 *
	 * @return a pointer to the buffer containing the md5 string, 0 if no tune is loaded.
	 */
	const char* createMD5 ( char* md5 = 0 );

	/**
	 * Calculates the MD5 hash of the tune, new method, introduced in HVSC#68.
	 * Not providing an md5 buffer will cause the internal one to be used.
	 * If provided, buffer must be MD5_LENGTH + 1
	 *
	 * @return a pointer to the buffer containing the md5 string, 0 if no tune is loaded.
	 */
	const char* createMD5New ( char* md5 = 0 );

	[[ nodiscard ]] const uint8_t* c64Data () const;
	[[ nodiscard ]] const std::vector<uint8_t>& getSidData () const;

private:
	// prevent copying
	SidTune ( const SidTune& ) = delete;
	SidTune& operator=( SidTune& ) = delete;
};
