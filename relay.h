#pragma once 


class Relay
{
private:
    bool state = false;
    int pin;
public:
    Relay() = default;
    void setup();

    bool isOn()const;
    void setPin(int relayPin);

    void off();
    void on();

};