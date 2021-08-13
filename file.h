#pragma once
#include "json/json.hpp"

#include <string>

class File
{
private:
    std::string fileName;
public:
    File(const std::string& newFileName);

    nlohmann::json read();
    void write(const nlohmann::json& content);     
};