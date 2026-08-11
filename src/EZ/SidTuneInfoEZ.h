#pragma once

/*
 * This file is part of libsidplayEZ, a SID player engine.
 *
 *  Copyright 2025-2026 Michael Hartmann
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
#include <string>
#include <vector>

namespace reSIDfp { enum class DigiMode : uint8_t; }

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
	std::string		clock = "PAL";

	// e.g. "CIA (PAL)", "50 Hz VBI (PAL)", etc.
	std::string		speed;

	// e.g. "Martin_Galway_Digi", "Rob_Hubbard", etc.
	std::vector<std::string>		playroutineID;

	// How the digi buffer derives its signed display-ready samples; the mode
	// implies the register the samples ride on. Zero value = nibble
	reSIDfp::DigiMode	digiMode {};

	// A dedicated sample player is present, digis are likely but not certain
	// (fx-only or digi-less subtunes exist)
	bool			digiPlayer = false;

	// e.g. "Martin Galway", "Rob Hubbard", "GoatTracker", etc.
	std::string			chipProfile;
	bool				chipProfileIsApproved = false;

	// The chip settings actually applied at load, audible fields only, fixed
	// order, deterministic text (consumers hash it for change detection)
	std::string			chipSettingsValues;

	// Stereo data
	int				stereoWidth = 0;	// in percent, from 0 to 100
	float			bassAdjust = 0.0f;	// in dB, usually negative values to push them down, as the defaults tend to be high
	bool			wantsStereo = false;	// The tune places its chips explicitly (PSID 4E), so the stereo image is authored

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
