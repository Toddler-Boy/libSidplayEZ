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

#include "stringutils.h"

#include <cstdint>
#include <sstream>
#include <fstream>

namespace stringutils
{

//-----------------------------------------------------------------------------

[[ nodiscard ]] bool equal ( const char* s1, const char* s2 )
{
	if ( s1 == s2 )
		return true;

	if ( s1 == 0 || s2 == 0 )
		return false;

	while ( *s1 || *s2 )
	{
		if ( ! casecompare ( *s1, *s2 ) )
			return false;

		++s1;
		++s2;
	}

	return true;
}
//-----------------------------------------------------------------------------

[[ nodiscard ]] bool equal ( const char* s1, const char* s2, size_t n )
{
	if ( s1 == s2 || n == 0 )
		return true;

	if ( s1 == 0 || s2 == 0 )
		return false;

	while ( n-- && ( *s1 || *s2 ) )
	{
		if ( ! casecompare ( *s1, *s2 ) )
			return false;

		++s1;
		++s2;
	}

	return true;
}
//-----------------------------------------------------------------------------

[[ nodiscard ]] std::string utf8toExtendedASCII ( const std::string& input )
{
	if ( ! input.size () )
		return {};

	auto	in = input.c_str ();
	std::string	out;

	while ( *in )
	{
		auto	ch = uint8_t ( *in++ );

		if ( ch == 0xC2 )		out.append ( 1, *in++ );
		else if ( ch == 0xC3 )	out.append ( 1, *in++ + 0x40 );
		else					out.append ( 1, char ( ch ) );
	}
	return out;
}
//-----------------------------------------------------------------------------

[[ nodiscard ]] std::string extendedASCIItoUTF8 ( const char* str )
{
	uint8_t	outBuffer[ 256 ];

	auto	in = str;
	auto	out = &outBuffer[ 0 ];

	while ( auto c = static_cast<uint8_t> ( *in++ ) )
	{
		if ( c < 128 )
		{
			*out++ = c;
		}
		else
		{
			*out++ = 0xC2 + ( c > 0xBF );
			*out++ = 0x80 + ( c & 0x3F );
		}
	}

	return std::string ( (const char*)&outBuffer[ 0 ], out - &outBuffer[ 0 ] );
}
//-----------------------------------------------------------------------------

[[ nodiscard ]] std::vector<std::string> arrayFromTokens ( const std::string& input, const char delimeter )
{
	std::vector<std::string>	tokens;

	std::stringstream	ss ( input );
	std::string			part;
	while ( std::getline ( ss, part, delimeter ) )
	{
		part = trim ( part );
		if ( ! part.empty () )
			tokens.emplace_back ( part );
	}

	return tokens;
}
//-----------------------------------------------------------------------------

[[ nodiscard ]] std::string loadFile ( const char* filename )
{
	// Read file into std::string
	auto	file = std::ifstream ( filename, std::ios::in | std::ios::binary | std::ios::ate );
	if ( ! file.is_open () )
		return {};

	const auto	size = file.tellg ();
	if ( size <= 0 )
		return {};

	file.seekg ( 0, std::ios::beg );

	auto	str = std::string ( size, '\0' );
	file.read ( str.data (), size );
	file.close ();

	return str;
}
//-----------------------------------------------------------------------------

} // namespace stringutils
