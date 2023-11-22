#pragma once
#include "json/json.hpp"

using json = nlohmann::basic_json<std::map, std::vector, std::string, bool, int, unsigned int, float>;