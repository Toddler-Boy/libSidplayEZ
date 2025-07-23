#pragma once

/*
 * This file is part of libsidplayEZ, a SID player engine.
 *
 *  Copyright 2025 Michael Hartmann
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

#include <string>
#include <vector>

//-----------------------------------------------------------------------------

struct SidTuneInfoEZ
{
	// All strings are stored as UTF8

	// Absolute path
	std::string		filename;

	// Tune info
	std::string		title;
	std::string		author;
	std::string		released;

	// 6581 or 8580
	std::vector<std::string>	model;

	// "PAL" or "NTSC"
	std::string		clock;

	// e.g. "CIA (PAL)", "50 Hz VBI (PAL)", etc.
	std::string		speed;

	// e.g. "Martin_Galway_Digi", "Rob_Hubbard", etc.
	std::vector<std::string>		playroutineID;

	// e.g. "Martin Galway", "Rob Hubbard", "GoatTracker", etc.
	std::string		chipProfile;

	// Stereo data
	int				stereoWidth = 0;	// in percent, from 0 to 100
	float			bassAdjust = 0.0f;	// in dB, usually negative values to push them down, as the defaults tend to be high

	// Technical data
	unsigned int	currentSong = 0;
	unsigned int	numSongs = 0;
	unsigned int	startSong = 0;
	std::string		md5;

	// C64 memory addresses
	uint16_t	c64LoadAddress = 0;
	uint16_t	c64InitAddress = 0;
	uint16_t	c64PlayAddress = 0;
	uint32_t	c64DataLength = 0;
};
//-----------------------------------------------------------------------------
