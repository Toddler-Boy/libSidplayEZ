#pragma once

#include <unordered_map>
#include <string>
#include <vector>

namespace libsidplayEZ
{

//-----------------------------------------------------------------------------

class TinyCSV final
{
public:
	int parseCSV ( const std::string& csvData );

	std::string getString ( const int row, const std::string& column, const std::string& defaultValue = "" );
	int getInt ( const int row, const std::string& column, const int defaultValue );
	double getDouble ( const int row, const std::string& column, const double defaultValue );

private:
	std::string	error;

	std::vector<std::unordered_map<std::string, std::string>>	data;
};
//-----------------------------------------------------------------------------

}