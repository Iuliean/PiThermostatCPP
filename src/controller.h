#pragma once

#include "relay.h"
#include "display.h"
#include "database.h"

#include <atomic>
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
public:    
    Controller(DataBase& app, const json& config, const json& parameters);
    virtual ~Controller() = default;

    virtual void run() = 0;

    Parameters getParameters();
    virtual bool getState()const = 0;
    
    void setParameters(float newMinTemp, float newMaxTemp);
    void setMinTemp(float newThreshold);
    void setMaxTemp(float newRange);
    
    void toDisk();
    
protected:      
    float   minTemp;
    float   maxTemp;
    float   temp;
    float   calibration;

    unsigned int     numOfReads;
    unsigned int     readDelay;
    unsigned int     saveInterval;

    std::mutex       parametersMutex;
    std::string      driverFile;

    DataBase& db;
    void checkTemp();
};

class HardwareController : public Controller
{
public:
    HardwareController(DataBase& db, const json& config, const json& parameters);
    virtual ~HardwareController()override = default;

    void run() override;
    inline bool getState()const override
    {
        return relay.isOn();
    }
private:
    Relay relay;
    Display display;

};

class HardwarelessController : public Controller
{
public:
    HardwarelessController(DataBase& db, const json& config, const json& parameters);
    virtual ~HardwarelessController()override = default;
    
    void run() override;
    inline bool getState()const override
    {
        return state;
    }
private:
    std::atomic_bool state;
};