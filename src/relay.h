#pragma once 
#include <atomic>

class Relay
{
private:
    std::atomic_bool state;
    std::atomic_int pin;
public:
    Relay();
    void setup();

    bool isOn()const;
    void setPin(int relayPin);

    void off();
    void on();

};