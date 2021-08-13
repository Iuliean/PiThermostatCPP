#include "controller.h"
#include "json/json.hpp"
#include "wiringPi.h"
#include "crow/crow_all.h"

#include <chrono>
#include <stdlib.h>
#include <stdint.h>
#include <thread>

#define MAX_TIMINGS	85
#define INVALID_TEMP -500
#define LOG_CONTROLLER_WARNING CROW_LOG_WARNING << "[CONTROLLER]:"
#define LOG_CONTROLLER_INFO CROW_LOG_INFO << "[CONTROLLER]:"

//Controller Class

Controller::Controller()
{
	wiringPiSetupGpio();

	nlohmann::json j = this->parametersFile.read();

	this->range 		= j["range"];
	this->threshold 	= j["threshold"];

	j = this->config.read();

	try{
		this->temp_pin 		= j["controller"]["tempPin"];
		this->tempReads		= j["controller"]["tempReads"];
		this->readDelay		= j["controller"]["readDelay"];

		if(this->tempReads == 0)
			throw 100;
		if(this->readDelay < 0)
			throw 101;
	}
	catch (int error)
	{
		switch(error)
		{
			case 100:	
				LOG_CONTROLLER_WARNING << "Can't have tempReads = 0. Will result in division by 0.Setting default 1.";
				this->tempReads	= 1;
				break;
			
			case 101:
				LOG_CONTROLLER_WARNING << "Can't have negative delay.Setting default 0.";
				this->readDelay;
		}
	}

	this->rel.setPin(j["controller"]["relayPin"]);
	this->rel.setup();
	this->disp.setSegments(j["controller"]["display"]);


}

void Controller::run()
{
	std::thread displayThread(&Display::run, &this->disp);

	std::chrono::time_point<std::chrono::high_resolution_clock> lastSave = std::chrono::high_resolution_clock::now();	

	while(true)
	{
		this->checkTemp();
		this->disp.show(this->temp);

		Parameters params = this->getParameters();

		if (params.temp > params.threshold + params.range)
			this->rel.off();
		else if (params.temp < params.threshold - params.range)
			this->rel.on();

		if(std::chrono::duration<float>(std::chrono::high_resolution_clock::now()- lastSave).count() > 600)
		{
			lastSave = std::chrono::high_resolution_clock::now();
			this->toDisk();
		}

		delay(this->readDelay);
	}

	displayThread.join();
}

Parameters Controller::getParameters()
{
	Parameters out;
	
	this->parametersMutex.lock();
	
	out.temp		= this->temp;
	out.threshold		= this->threshold;
	out.range		= this->range;
	out.state		= this->getState();
	
	this->parametersMutex.unlock();

	return out;
}

bool Controller::getState()const
{
	return this->rel.isOn();
}

void Controller::setParameters(float newThreshold, float newRange)
{
	this->parametersMutex.lock();
	
	this->range	= newRange;
	this->threshold = newThreshold;

	this->parametersMutex.unlock();
}

void Controller::setThreshold(float newThreshold)
{
	this->parametersMutex.lock();

	this->threshold = newThreshold;

	this->parametersMutex.unlock();
}

void Controller::setRange(float newRange)
{
	this->parametersMutex.lock();

	this->range = newRange;

	this->parametersMutex.unlock();
}

//Private funcs

void Controller::toDisk()
{
	LOG_CONTROLLER_INFO << "Writing parameters to disk >> parameters.json";
	nlohmann::json j;

	j["range"]		= double(int(this->range * 10))/10;
	j["threshold"]		= double(int(this->threshold * 10))/10;

	this->parametersFile.write(j);
}

float Controller::querryTempSensor()
{
	int data[5] = { 0, 0, 0, 0, 0 };
	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j			= 0, i;
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	
	pinMode(this->temp_pin, OUTPUT);
	digitalWrite(this->temp_pin, LOW);
	delay( 18 );
	
	pinMode(this->temp_pin, INPUT);
	
	for ( i = 0; i < MAX_TIMINGS; i++ )
	{
		counter = 0;
		while (digitalRead(this->temp_pin) == laststate)
		{
			counter++;
			delayMicroseconds( 1 );
			if ( counter == 255 )
			{
				break;
			}
		}
		laststate = digitalRead(this->temp_pin);
		if ( counter == 255 )
			break;

		if ( (i >= 4) && (i % 2 == 0) )
		{

			data[j / 8] <<= 1;
			if ( counter > 16 )
				data[j / 8] |= 1;
			j++;
		}
	}

	if ( (j >= 40) &&
	     (data[4] == ( (data[0] + data[1] + data[2] + data[3]) & 0xFF) ) )
	{
		float h = (float)((data[0] << 8) + data[1]) / 10;
		if ( h > 100 )
		{
			h = data[0];
		}
		float c = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10;
		if ( c > 125 )
		{
			c = data[2]; 
		}
		if ( data[2] & 0x80 )
		{
			c = -c;
		}
		float f = c * 1.8f + 32;
		return c;
	}else  {
		return INVALID_TEMP;
	}
}

void Controller::checkTemp()
{
	float sum = 0;

	for(int i = 0; i < this->tempReads; i++)
	{
		float newTemp = this->querryTempSensor();
		if(newTemp != INVALID_TEMP)
		{
			sum += newTemp;
		}
		else
		{
			while(newTemp == INVALID_TEMP)
			{
				delay(2000);
				newTemp = this->querryTempSensor();
			}

			sum += newTemp;
		}
	}

	this->temp = sum/this->tempReads;

}