#pragma once
/*
* This file is part of libsidplayEZ, a SID player engine.
*
* Copyright 2026 Michael Hartmann
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

//-----------------------------------------------------------------------------

#include <memory>
#include <string>

#include "audio-profile-selector.h"
#include "chip-profile-selector.h"
#include "override-selector.h"
#include "sidid.h"

namespace libsidplayEZ
{

/**
* All parsed configuration data (sidid player signatures, chip/audio profiles,
* tune overrides) bundled into one shareable object, in the same spirit as
* SharedFilterTables: parse once via the four load functions, then the same
* shared_ptr<const SharedPlayerConfig> is handed to every Player instance.
* Pure data after loading - all accessors are const, so concurrent Players can
* read it without locking.
*
* The four configs load individually, so a single changed file can be reloaded
* without re-parsing the rest: copy the current config, reload the changed part
* on the copy, then hand the new pointer to future Players (instances keep
* whatever config they were given, so nothing changes under a running Player).
*/
struct SharedPlayerConfig final
{
	sidid					sidID;
	ChipProfileSelector		chipSelector;
	AudioProfileSelector	audioSelector;
	OverrideSelector		overrideSelector;

	bool loadSidIDConfig ( const char* filename )			{ return sidID.loadSidIDConfig ( filename ); }
	void loadChipProfiles ( const std::string& csvStr )		{ chipSelector.setProfiles ( csvStr ); }
	void loadAudioProfiles ( const std::string& csvStr )	{ audioSelector.setProfiles ( csvStr ); }
	void loadTuneOverrides ( const std::string& csvStr )	{ overrideSelector.setOverrides ( csvStr ); }
};
//-----------------------------------------------------------------------------

}
