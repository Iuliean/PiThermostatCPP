#include "relay.h"

#include <wiringPi.h>

Relay::Relay()
{
    state = false;
}

void Relay::setup()
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

bool Relay::isOn()const
{
    return state;
}

void Relay::setPin(int relayPin)
{
    pin = relayPin;
}

void Relay::off()
{
    digitalWrite(pin, HIGH);
    state = false;
}

void Relay::on()
{
    digitalWrite(pin, LOW);
    state = true;   
}