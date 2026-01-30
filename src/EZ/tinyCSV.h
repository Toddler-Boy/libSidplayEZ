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

private:
	std::string	error;

	std::vector<std::unordered_map<std::string, std::string>>	data;
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
		return it->second;
	else if constexpr ( std::is_same_v<T, int> )
		return std::stoi ( it->second );
	else if constexpr ( std::is_same_v<T, float> )
		return std::stof ( it->second );
	else if constexpr ( std::is_same_v<T, double> )
		return std::stod ( it->second );

	assert ( false ); // Unsupported type

	return defaultValue;
}
//-----------------------------------------------------------------------------

inline std::string TinyCSV::get ( const int row, const std::string& column, const char* defaultValue ) const
{
	return get<std::string> ( row, column, defaultValue );
}
//-----------------------------------------------------------------------------

}