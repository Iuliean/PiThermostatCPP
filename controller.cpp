#include "controller.h"
#include "json_custom.h"
#include "wiringPi.h"
#include "crow/crow_all.h"
#include "database.h"

#include <chrono>
#include <thread>
#include<iostream>

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
		this->numOfReads	= j["controller"]["numOfReads"];
		this->readDelay		= j["controller"]["readDelay"];
		this->saveInterval	= j["controller"]["saveInterval"];
		this->calibration 	= j["controller"]["calibration"];
		this->driverFile    = j["controller"]["driverFile"];

		if(this->numOfReads <= 0)
			throw 100;
		if(this->readDelay < 0)
			throw 101;
	}
	catch (int error)
	{
		switch(error)
		{
			case 100:	
				LOG_CONTROLLER_WARNING << "Can't have numOfReads <= 0. Will result in division by 0 or invalid temperatures. Setting default 1.";
				this->numOfReads	= 1;
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

void Controller::checkTemp()
{
	int sum = 0;

	for (int i = 0; i < this->numOfReads; i++)
	{
		std::ifstream sensorFile (this->driverFile);
		int t;
		sensorFile >> t;
		sum += t;

		std::this_thread::sleep_for(std::chrono::milliseconds(this->readDelay/this->numOfReads));
	}

	sum /= 100 * this->numOfReads;

	temp = ((float)sum / 10) + this->calibration;
}
