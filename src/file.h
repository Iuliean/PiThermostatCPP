#pragma once
#include "json_custom.h"

#include <string>

class File
{
private:
    std::string fileName;
public:
    File(const std::string& newFileName);

    json read();
    void write(const json& content);     
};