/*
* This file is part of libsidplayfp, a SID player engine.
*
* Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
* Copyright 2007-2010 Antti Lankila
* Copyright 2004,2010 Dag Lem <resid@nimrod.no>
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

#include "Filter6581.h"

namespace reSIDfp
{
//-----------------------------------------------------------------------------

Filter6581::Filter6581 ()
	: Filter ( *FilterModelConfig6581::getInstance () )
	, fmc6581 ( *FilterModelConfig6581::getInstance () )
	, f0_dac ( fmc6581.getDAC ( 0.5 ) )
	, hpIntegrator ( fmc6581 )
	, bpIntegrator ( fmc6581 )
{
	setFilterCurve ( 0.5f );

	updatedCenterFrequency ();

	input ( 0 );
}
//-----------------------------------------------------------------------------

void Filter6581::setFilterCurve ( double curvePosition )
{
	delete[] f0_dac;
	f0_dac = fmc6581.getDAC ( curvePosition );
	updatedCenterFrequency ();
}
//-----------------------------------------------------------------------------

void Filter6581::setFilterRange ( double adjustment )
{
	fmc6581.setFilterRange ( adjustment );
}
//-----------------------------------------------------------------------------

void Filter6581::setFilterGain ( double adjustment )
{
	filterGain = int ( adjustment * ( 1 << 12 ) );
	filterOffset = 32767 * ( ( 1 << 12 ) - filterGain );
}
//-----------------------------------------------------------------------------

void Filter6581::setDigiVolume ( double adjustment )
{
	Ve = int16_t ( adjustment * fmc6581.getNormalizedVoice ( 0.0f, 0 ) );
}
//-----------------------------------------------------------------------------

void Filter6581::setVoiceDCDrift ( double adjustment )
{
	fmc6581.setVoiceDCDrift ( adjustment );
}
//-----------------------------------------------------------------------------

} // namespace reSIDfp
