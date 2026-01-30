#pragma once

/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2013-2023 Leandro Nini
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

#include <cctype>
#include <algorithm>
#include <string>
#include <vector>

namespace stringutils
{
	/**
	 * Compare two characters in a case insensitive way.
	 */
	[[ nodiscard ]] inline bool casecompare ( char c1, char c2 ) { return std::tolower ( c1 ) == std::tolower ( c2 ); }

	/**
	 * Compare two strings in a case insensitive way.
	 *
	 * @return true if strings are equal.
	 */
	[[ nodiscard ]] inline bool equal ( const std::string& s1, const std::string& s2 )
	{
		return std::equal ( s1.begin (), s1.end (), s2.begin (), s2.end (), casecompare );
	}

	/**
	 * Compare two strings in a case insensitive way.
	 *
	 * @return true if strings are equal.
	 */
	[[ nodiscard ]] bool equal ( const char* s1, const char* s2 );

	/**
	 * Compare first n characters of two strings in a case insensitive way.
	 *
	 * @return true if strings are equal.
	 */
	[[ nodiscard ]] bool equal ( const char* s1, const char* s2, size_t n );

	[[ nodiscard ]] inline std::string toLower ( const std::string& input )
	{
		auto	newStr = input;

		std::transform ( newStr.begin (), newStr.end (), newStr.begin (), [] ( unsigned char c ) { return std::tolower ( c ); } );

		return newStr;
	}

	[[ nodiscard ]] inline std::string toUpper ( const std::string& input )
	{
		auto	newStr = input;

		std::transform ( newStr.begin (), newStr.end (), newStr.begin (), [] ( unsigned char c ) { return std::toupper ( c ); } );

		return newStr;
	}

	[[ nodiscard ]] std::string utf8toExtendedASCII ( const std::string& input );
	[[ nodiscard ]] std::string extendedASCIItoUTF8 ( const char* str );

	[[ nodiscard ]] inline std::string trim ( std::string str )
	{
		// ltrim
		str.erase ( str.begin (), std::find_if ( str.begin (), str.end (), [] ( unsigned char ch ) {	return ! std::isspace ( ch );	} ) );
		// rtrim
		str.erase ( std::find_if ( str.rbegin (), str.rend (), [] ( unsigned char ch ) { return ! std::isspace ( ch );	} ).base (), str.end () );

		return str;
	}

	[[ nodiscard ]] std::vector<std::string> arrayFromTokens ( const std::string& input, const char delimeter = '\n' );
	[[ nodiscard ]] std::string loadFile ( const char* filename );
} // namespace stringutils
