#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <cassert>

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

class TinyCSV final
{
public:
	int parseCSV ( const std::string& csvData );

	template<typename T>
	T get ( const int row, const std::string& column, T defaultValue = {} ) const;
	std::string get ( const int row, const std::string& column, const char* defaultValue = "" ) const;

	// Empty when the whole file parsed and converted cleanly. Names the first bad cell,
	// which keeps its default value
	[[ nodiscard ]] const std::string& getError () const	{	return error;	}

private:
	mutable std::string	error;

	std::vector<std::unordered_map<std::string, std::string>>	data;

	// Source line and its number for each data row, so an error can point at the file
	// rather than at a row index the editor knows nothing about
	std::vector<std::string>	rawLines;
	std::vector<int>			lineNumbers;
};
//-----------------------------------------------------------------------------

template<typename T>
inline T TinyCSV::get ( const int row, const std::string& column, T defaultValue ) const
{
	if ( size_t ( row ) >= data.size () )
		return defaultValue;

	auto	it = data[ row ].find ( column );
	if ( it == data[ row ].end () )
		return defaultValue;

	if constexpr ( std::is_same_v<T, std::string> )
	{
		return it->second;
	}
	else
	{
		static_assert ( std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double>, "Unsupported type" );

		// A hand-edited file can hold anything. The conversions throw when nothing at the
		// front is a number, but keep the leading digits of something like "1.1x" without
		// complaint, so the whole cell has to be accounted for. Empty cells never get here,
		// parseCSV drops them and the caller's default applies
		try
		{
			size_t	used = 0;
			T		value {};

			if constexpr ( std::is_same_v<T, int> )			value = std::stoi ( it->second, &used );
			else if constexpr ( std::is_same_v<T, float> )	value = std::stof ( it->second, &used );
			else if constexpr ( std::is_same_v<T, double> )	value = std::stod ( it->second, &used );

			if ( used == it->second.size () )
				return value;
		}
		catch ( ... )
		{
		}

		if ( error.empty () )
			error = "line " + std::to_string ( lineNumbers[ row ] ) + ", column '" + column + "' holds '" + it->second
					+ "', which is not a number\n" + rawLines[ row ];

		return defaultValue;
	}
}
//-----------------------------------------------------------------------------

inline std::string TinyCSV::get ( const int row, const std::string& column, const char* defaultValue ) const
{
	return get<std::string> ( row, column, defaultValue );
}
//-----------------------------------------------------------------------------

}