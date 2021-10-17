#pragma once

#include "file.h"
#include "relay.h"
#include "display.h"

#include <mutex>

struct Parameters
{
    float   minTemp;
    float   maxTemp;
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

    float   minTemp;
    float   maxTemp;
    float   temp;

    unsigned int     temp_pin;
    unsigned int     tempReads;
    unsigned int     readDelay;
    unsigned int     saveInterval;

    std::mutex parametersMutex;
public:
    Controller();

    void run();


    Parameters getParameters();
    bool getState()const;
    
    void setParameters(float newMinTemp, float newMaxTemp);
    void setMinTemp(float newThreshold);
    void setMaxTemp(float newRange);
    
    void toDisk();
private:
    void checkTemp();
    
    float querryTempSensor();
};