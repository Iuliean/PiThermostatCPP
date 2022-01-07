#pragma once
#include "json_custom.h"

#include <vector>
#include <string>
#include <map>

class Display
{
private:
    std::string number = "00.0";
    std::map<std::string, unsigned int> segments;
    std::vector<int> digitPins;
    unsigned int refreshRate;
public:
    Display() = default;

    void run();

    void setSegments(const json& segs);
    
    inline void setRefreshRate(unsigned int newRefreshRate)
    {
        this->refreshRate = newRefreshRate;
    }
    
    inline void show(float temp)
    {
        this->number = std::to_string(temp);
        this->number = this->number.substr(0,this->number.find('.')+2);
    }
    
    inline void show(const std::string& temp)
    {
        this->number = temp;
    }

private:
    void pinsInit()const;
    void clearSegments()const;
};