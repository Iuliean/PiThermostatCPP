#include "file.h"

#include <fstream>

File::File(const std::string& newFileName)
{
	fileName = newFileName;
}

json File::read()
{
	std::ifstream file(fileName);
	json out;

	file >> out;
	return out;
}

void File::write(const json& content)
{
	std::ofstream file(fileName);

	file << content;
}
