#include "controller.h"
#include "display.h"
#include "json_custom.h"
#include "relay.h"
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

Controller::Controller(DataBase& odb, const json& config, const json& parameters)
	: db(odb)
{
	minTemp		= parameters["minTemp"];
	maxTemp 	= parameters["maxTemp"];

	numOfReads		= config["numOfReads"];
	readDelay		= config["readDelay"];
	saveInterval	= config["saveInterval"];
	calibration 	= config["calibration"];
	driverFile		= config["driverFile"];

	if (numOfReads <= 0)
	{
		LOG_CONTROLLER_WARNING << "Can't have numOfReads <= 0. Setting default of 4";
		numOfReads = 4;
	}
	if (readDelay < 0)
	{
		LOG_CONTROLLER_WARNING << "Can't have negative delay. Setting default of 2000";
		readDelay = 2000;
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

	std::ofstream out("parameters.json");
	out << j.dump(4);
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

HardwareController::HardwareController(DataBase& db, const json& config, const json& parameters)
	: Controller(db, config, parameters),
	relay(),
	display()
{
	wiringPiSetupGpio();
	
	relay.setPin(config["relayPin"]);
	relay.setup();
	
	display.setSegments(config["display"]);
	display.setRefreshRate(config["display"]["refreshRate"]);
}

void HardwareController::run()
{
	std::thread displayThread(&Display::run, &display);
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
		display.show(params.temp);

		if (params.temp > params.maxTemp && relay.isOn())
		{
			relay.off();
			LOG_CONTROLLER_INFO << "Current temp is: " << temp << " switch turned off";

			db.insertState(
				true,
				std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - stateTime).count()
			);

			stateTime = std::chrono::high_resolution_clock::now();
		}
		else if (params.temp < params.minTemp && !relay.isOn())
		{
			relay.on();
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

HardwarelessController::HardwarelessController(DataBase& db, const json& config, const json& parameters)
	: Controller(db, config, parameters),
	state(false)
{
}

void HardwarelessController::run()
{
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

		if (params.temp > params.maxTemp && state)
		{
			state = true;
			LOG_CONTROLLER_INFO << "Current temp is: " << temp << " switch turned off";

			db.insertState(
				true,
				std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - stateTime).count()
			);

			stateTime = std::chrono::high_resolution_clock::now();
		}
		else if (params.temp < params.minTemp && !state)
		{
			state = true;
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