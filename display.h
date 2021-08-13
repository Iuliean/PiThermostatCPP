#pragma once
#include "json/json.hpp"

#include <vector>
#include <string>
#include <map>

class Display
{
private:
    std::string number = "00.0";
    std::map<std::string, unsigned int> segments;
    std::vector<int> digitPins;

    //std::mutex tempMutex;
public:
    Display() = default;

    void run();

    void setSegments(const nlohmann::json& segs);
    void show(float temp);
    void show(const std::string& temp);

private:
    void pinsInit()const;
    void clearSegments()const;
};