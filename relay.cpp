#include "relay.h"

#include <wiringPi.h>

void Relay::setup()
{
    pinMode(this->pin, OUTPUT);
    digitalWrite(this->pin, HIGH);
}

bool Relay::isOn()const
{
    return this->state;
}

void Relay::setPin(int relayPin)
{
    this->pin = relayPin;
}

void Relay::off()
{
    digitalWrite(this->pin, HIGH);
    this->state = false;
}

void Relay::on()
{
    digitalWrite(this->pin, LOW);
    this->state = true;   
}