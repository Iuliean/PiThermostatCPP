#include "controller.h"
#include "json_custom.h"
#include "wiringPi.h"
#include "crow/crow_all.h"
#include "database.h"

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

	json j = this->parametersFile.read();

	this->minTemp		= j["minTemp"];
	this->maxTemp 		= j["maxTemp"];

	j = this->config.read();

	try{
		this->temp_pin 		= j["controller"]["tempPin"];
		this->tempReads		= j["controller"]["tempReads"];
		this->readDelay		= j["controller"]["readDelay"];
		this->saveInterval	= j["controller"]["saveInterval"];

		if(this->tempReads <= 0)
			throw 100;
		if(this->readDelay < 0)
			throw 101;
	}
	catch (int error)
	{
		switch(error)
		{
			case 100:	
				LOG_CONTROLLER_WARNING << "Can't have tempReads <= 0. Will result in division by 0 or invalid temperatures. Setting default 1.";
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
	this->disp.setRefreshRate(j["controller"]["display"]["refreshRate"]);


}

void Controller::run()
{
	std::thread displayThread(&Display::run, &this->disp);
	std::thread tempSaveRoutine ([this](){
		DataBase& db = DataBase::getInstance();
		while(true)
		{
			std::this_thread::sleep_for(std::chrono::minutes(10));
			db.insertTemp(this->getParameters().temp);
		}		
	});

	std::chrono::time_point<std::chrono::high_resolution_clock> lastSave = std::chrono::high_resolution_clock::now();	
	std::chrono::time_point<std::chrono::high_resolution_clock> stateTime= std::chrono::high_resolution_clock::now();
	
	DataBase& db = DataBase::getInstance();

	while(true)
	{
		this->checkTemp();
		Parameters params = this->getParameters();
		this->disp.show(params.temp);
		
		if (params.temp > params.maxTemp && this->rel.isOn())
		{
			this->rel.off();
			LOG_CONTROLLER_INFO << "Current temp is: " << this->temp << " switch turned off";

			db.insertState(
				true,
				std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - stateTime).count()
			);

			stateTime = std::chrono::high_resolution_clock::now();
		}
		else if (params.temp < params.minTemp && !this->rel.isOn())
		{
			this->rel.on();
			LOG_CONTROLLER_INFO << "Current temp is:" << this->temp << " switch turned on";

			db.insertState(
				false,
				std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - stateTime).count()
			);

			stateTime = std::chrono::high_resolution_clock::now();
		}

		if(std::chrono::duration<float>(std::chrono::high_resolution_clock::now()- lastSave).count() > saveInterval)
		{
			lastSave = std::chrono::high_resolution_clock::now();
			this->toDisk();
		}

		delay(this->readDelay);
	}

}

Parameters Controller::getParameters()
{
	Parameters out;
	
	this->parametersMutex.lock();
	
	out.temp		= this->temp;
	out.minTemp		= this->minTemp;
	out.maxTemp		= this->maxTemp;
	out.state		= this->getState();
	
	this->parametersMutex.unlock();

	return out;
}

bool Controller::getState()const
{
	return this->rel.isOn();
}

void Controller::setParameters(float newMinTemp, float newMaxTemp)
{
	this->parametersMutex.lock();
	
	this->minTemp		= newMinTemp;
	this->maxTemp 		= newMaxTemp;

	this->parametersMutex.unlock();
}

void Controller::setMinTemp(float newMinTemp)
{
	this->parametersMutex.lock();

	this->minTemp 		= newMinTemp;

	this->parametersMutex.unlock();
}

void Controller::setMaxTemp(float newMaxTemp)
{
	this->parametersMutex.lock();

	this->maxTemp 		= newMaxTemp;

	this->parametersMutex.unlock();
}

//Private funcs

void Controller::toDisk()
{
	LOG_CONTROLLER_INFO << "Writing parameters to disk >> parameters.json";
	json j;

	j["minTemp"]		= this->minTemp;
	j["maxTemp"]		= this->maxTemp;

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
		if(newTemp > -200)
		{
			sum += newTemp;
		}
		else
		{
			while(newTemp < -200)
			{
				delay(2000);
				newTemp = this->querryTempSensor();
			}

			sum += newTemp;
		}
	}

	this->temp = sum/this->tempReads;

}
