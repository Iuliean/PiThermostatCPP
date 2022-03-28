#pragma once
#include "json_custom.h"

#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <mutex>

class Display
{
private:
    std::string number = "00.0";
    std::mutex numberMutex;

    std::map<std::string, unsigned int> segments;
    std::vector<int> digitPins;
    std::atomic<unsigned int> refreshRate;
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
        std::lock_guard<std::mutex> l (this->numberMutex);
        this->number = std::to_string(temp);
        this->number = this->number.substr(0,this->number.find('.')+2);
    }

private:
    void pinsInit()const;
    void clearSegments()const;
};