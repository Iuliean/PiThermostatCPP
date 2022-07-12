#include "crow/logging.h"
#include "display.h"

#include <wiringPi.h>

#define LOG_DISPLAY_INFO CROW_LOG_INFO << "[DISPLAY]:"


static const std::map<char, std::vector<char>> numberConstructor = {
    {'1',{'B','C'}},
    {'2',{'A','B','G','E','D'}},
    {'3',{'A','B','G','C','D'}},
    {'4',{'F','B','G','C'}},
    {'5',{'A','F','G','C','D'}},
    {'6',{'F','E','D','C','G','A'}},
    {'7',{'A','B','C'}},
    {'8',{'A','B','C','D','E','F','G'}},
    {'9',{'A','B','C','F','G','D'}},
    {'0',{'A','B','C','E','F','D'}},
    {'-',{'G'}},
};

//Display
void Display::run()
{
    bool decimalPoint;
    pinsInit();

    while(true)
    {
        decimalPoint = false;
        int posShift = 0;

        std::string temperature = number;
        for(int i = temperature.length() - 1; i > -1; i--)
        {
            const char& digit = temperature[i];

            clearSegments();
            if(digit == '.')
            {
                decimalPoint = true;
                continue;
            }
            
            for(const char& segment : numberConstructor.at(digit))
            {
                digitalWrite(segments[std::string(1,segment)], HIGH);
            }

            if(decimalPoint)
            {
                decimalPoint = false;
                digitalWrite(segments["DP"], HIGH);
            }

            digitalWrite(digitPins[posShift], LOW);
            delay(refreshRate.load());
            digitalWrite(digitPins[posShift], HIGH);
            
            posShift++;

        }
    }

}

void Display::setSegments(const json& segs)
{
    for(const unsigned int& i : segs["digits"])
        digitPins.push_back(i);

    segments["A"]     = segs["A"];
    segments["B"]     = segs["B"];
    segments["C"]     = segs["C"];
    segments["D"]     = segs["D"];
    segments["E"]     = segs["E"];
    segments["F"]     = segs["F"];
    segments["G"]     = segs["G"];
    segments["DP"]    = segs["DP"];

    for(auto it = segments.begin(); it != segments.end(); it++)
    {
        LOG_DISPLAY_INFO << "Segment "<<it->first <<" set to pin " <<it->second;
    }
}

//Private
void Display::pinsInit()const
{
    for(const int& pin : digitPins)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);

        LOG_DISPLAY_INFO << "Initializing digit pin: " << pin << " and settings the output LOW";
    }

    for(auto& it : segments)
    {
        pinMode(it.second, OUTPUT);
        digitalWrite(it.second,LOW);

        LOG_DISPLAY_INFO << "Initializing pin: " << it.second << " and setting the output LOW";
    }
}

void Display::clearSegments()const
{
    for(auto& it : segments)
    {
        digitalWrite(it.second, LOW);
    }
}