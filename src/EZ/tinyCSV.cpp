#include "tinyCSV.h"

#include <iostream>
#include <string>
#include <sstream>

#include "../stringutils.h"

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

int TinyCSV::parseCSV ( const std::string& csvData )
{
	data.clear ();

	error = "Empty CSV data";
	if ( csvData.empty () )
		return 0;

	auto	iss = std::istringstream ( csvData );
	if ( iss.eof () )
		return 0;
	
	error = "";

	auto parseLine = [] ( const std::string& row ) -> std::vector<std::string>
	{
		std::vector<std::string>	fields;
		std::string					currentField;
		auto						inQuotes = false;

		for ( size_t i = 0; i < row.length (); ++i )
		{
			const auto	c = row[ i ];

			if ( c == '"' )
			{
				if ( inQuotes && ( i + 1 < row.length () ) && row[ i + 1 ] == '"' )
				{
					currentField += '"';
					++i;
				}
				else
				{
					inQuotes = ! inQuotes;
				}
			}
			else if ( c == ',' && ! inQuotes )
			{
				fields.emplace_back ( stringutils::trim ( currentField ) );
				currentField.clear ();
			}
			else
			{
				currentField += c;
			}
		}

		fields.emplace_back ( stringutils::trim ( currentField ) );

		return fields;
	};

	std::vector<std::string>	columns;
	std::string					line;

	while ( std::getline ( iss, line ) )
	{
		if ( line.empty () || line[ 0 ] == '#' )
			continue;

		if ( columns.empty () )
		{
			columns = parseLine ( line );
			continue;
		}

		auto	values = parseLine ( line );

		std::unordered_map<std::string, std::string>	lineData;

		for ( size_t i = 0; i < std::min ( columns.size (), values.size () ); ++i )
			if ( ! values[ i ].empty () )
				lineData[ columns[ i ] ] = values[ i ];

		data.emplace_back ( lineData );
	}

	return int ( data.size () );
}
//-----------------------------------------------------------------------------

std::string TinyCSV::getString ( const int row, const std::string& column, const std::string& defaultValue )
{
	if ( size_t ( row ) >= data.size () )
		return defaultValue;

	auto	it = data[ row ].find ( column );
	if ( it == data[ row ].end () )
		return defaultValue;

	return it->second;
}
//-----------------------------------------------------------------------------

int TinyCSV::getInt ( const int row, const std::string& column, const int defaultValue )
{
	const auto	str = getString ( row, column, std::to_string ( defaultValue ) );

	return std::stoi ( str );
}
//-----------------------------------------------------------------------------

double TinyCSV::getDouble ( const int row, const std::string& column, const double defaultValue )
{
	const auto	str = getString ( row, column, std::to_string ( defaultValue ) );

	return std::stod ( str );
}
//-----------------------------------------------------------------------------

}