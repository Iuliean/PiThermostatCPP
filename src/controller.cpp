#include "controller.h"
#include "json_custom.h"
#include "wiringPi.h"
#include "crow/logging.h"
#include "database.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <iostream>
#include <fstream>

#define LOG_CONTROLLER_WARNING CROW_LOG_WARNING << "[CONTROLLER]:"
#define LOG_CONTROLLER_INFO CROW_LOG_INFO << "[CONTROLLER]:"


//Controller Class

Controller::Controller(DataBase& odb)
	: config("config.json"),
	parametersFile("parameters.json"),
	db(odb)
	
{
	wiringPiSetupGpio();
	
	json j = parametersFile.read();

	minTemp		= j["minTemp"];
	maxTemp 	= j["maxTemp"];

	j = config.read()["controller"];

	numOfReads		= j["numOfReads"];
	readDelay		= j["readDelay"];
	saveInterval	= j["saveInterval"];
	calibration 	= j["calibration"];
	driverFile    	= j["driverFile"];

	if (numOfReads <= 0)
	{
		LOG_CONTROLLER_WARNING << "Can't have numOfReads <= 0. Setting default of 4";
		numOfReads = 4;
	}
	if(readDelay < 0)
	{
		LOG_CONTROLLER_WARNING << "Can't have negative delay. Setting default of 2000";
		readDelay = 2000;
	}

	rel.setPin(j["relayPin"]);
	rel.setup();
	disp.setSegments(j["display"]);
	disp.setRefreshRate(j["display"]["refreshRate"]);


}

void Controller::run()
{
	std::thread displayThread(&Display::run, &disp);
	std::thread tempSaveRoutine ([this](){
		while(true)
		{
			std::this_thread::sleep_for(std::chrono::minutes(10));
			db.insertTemp(getParameters().temp);
		}
	});

	std::chrono::time_point<std::chrono::high_resolution_clock> lastSave = std::chrono::high_resolution_clock::now();	
	std::chrono::time_point<std::chrono::high_resolution_clock> stateTime= std::chrono::high_resolution_clock::now();
	
	while(true)
	{
		checkTemp();
		Parameters params = getParameters();
		disp.show(params.temp);

		if (params.temp > params.maxTemp && rel.isOn())
		{
			rel.off();
			LOG_CONTROLLER_INFO << "Current temp is: " << temp << " switch turned off";

			db.insertState(
				true,
				std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - stateTime).count()
			);

			stateTime = std::chrono::high_resolution_clock::now();
		}
		else if (params.temp < params.minTemp && !rel.isOn())
		{
			rel.on();
			LOG_CONTROLLER_INFO << "Current temp is:" << temp << " switch turned on";

			db.insertState(
				false,
				std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - stateTime).count()
			);

			stateTime = std::chrono::high_resolution_clock::now();
		}

		if(std::chrono::duration<float>(std::chrono::high_resolution_clock::now()- lastSave).count() > saveInterval)
		{
			lastSave = std::chrono::high_resolution_clock::now();
			toDisk();
		}
	}
}

Parameters Controller::getParameters()
{
	Parameters out;
	std::lock_guard<std::mutex> l(parametersMutex);
	
	out.temp		= temp;
	out.minTemp		= minTemp;
	out.maxTemp		= maxTemp;
	out.state		= getState();

	return out;
}

bool Controller::getState()const
{
	return rel.isOn();
}

void Controller::setParameters(float newMinTemp, float newMaxTemp)
{
	parametersMutex.lock();
	
	minTemp		= newMinTemp;
	maxTemp 		= newMaxTemp;

	parametersMutex.unlock();
}

void Controller::setMinTemp(float newMinTemp)
{
	parametersMutex.lock();

	minTemp 		= newMinTemp;

	parametersMutex.unlock();
}

void Controller::setMaxTemp(float newMaxTemp)
{
	parametersMutex.lock();

	maxTemp 		= newMaxTemp;

	parametersMutex.unlock();
}

//Private funcs

void Controller::toDisk()
{
	LOG_CONTROLLER_INFO << "Writing parameters to disk >> parameters.json";
	json j;

	j["minTemp"]		= minTemp;
	j["maxTemp"]		= maxTemp;

	parametersFile.write(j);
}

void Controller::checkTemp()
{
	int sum = 0;

	for (int i = 0; i < numOfReads; i++)
	{
		std::ifstream sensorFile (driverFile);
		int t;
		sensorFile >> t;
		sum += t;

		std::this_thread::sleep_for(std::chrono::milliseconds(readDelay/numOfReads));
	}

	sum /= 100 * numOfReads;

	temp = ((float)sum / 10) + calibration;
}
