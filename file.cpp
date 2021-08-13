#include "file.h"

#include <fstream>

File::File(const std::string& newFileName)
{
	this->fileName = newFileName;
}

nlohmann::json File::read()
{
	std::ifstream file(this->fileName);
	nlohmann::json out;

	file >> out;
	return out;
}

void File::write(const nlohmann::json& content)
{
	std::ofstream file(this->fileName);

	file << content;
}
