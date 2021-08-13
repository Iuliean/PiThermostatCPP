#pragma once

#include "file.h"
#include "relay.h"
#include "display.h"

#include <mutex>

struct Parameters
{
    float   threshold;
    float   range;
    float   temp;
    bool    state;
};


class Controller
{
private:
    File    parametersFile{"parameters.json"};
    File    config{"config.json"};
    
    Relay   rel;
    Display disp;

    float   threshold;
    float   range;
    float   temp;

    unsigned int     temp_pin;
    unsigned int     tempReads;
    unsigned int     readDelay;

    std::mutex parametersMutex;
public:
    Controller();

    void run();


    Parameters getParameters();
    bool getState()const;
    
    void setParameters(float newThreshold, float newRange);
    void setThreshold(float newThreshold);
    void setRange(float newRange);
private:
    void toDisk();
    void checkTemp();
    
    float querryTempSensor();
};