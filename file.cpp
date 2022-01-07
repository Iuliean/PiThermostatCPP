#include "file.h"

#include <fstream>

File::File(const std::string& newFileName)
{
	this->fileName = newFileName;
}

json File::read()
{
	std::ifstream file(this->fileName);
	json out;

	file >> out;
	return out;
}

void File::write(const json& content)
{
	std::ofstream file(this->fileName);

	file << content;
}
