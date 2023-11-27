#include "app.h"
#include "controller.h"
#include "crow/logging.h"
#include "json_custom.h"

#include "json/json.hpp"
#include <thread>

#define LOG_APP_INFO CROW_LOG_INFO <<"[APP]:"

App::App()
    : db()
{
    std::ifstream configFile("config.json");
    std::ifstream parametersFile("parameters.json");

    json config = nlohmann::json::parse(configFile);
    json parameters = nlohmann::json::parse(parametersFile);

    if(config["hardwareless"])
    {
        ctrl = new HardwarelessController(db, config["controller"], parameters);
    }
    else
    {
        ctrl = new HardwareController(db, config["controller"], parameters);
    }
    site = new Site(db, *ctrl, config["site"]);
}

App::~App()
{
    delete site;
    delete ctrl;
}

void App::run()
{  
    LOG_APP_INFO << "Starting controller thread";
    std::thread controllerThread(&Controller::run, ctrl);

    LOG_APP_INFO <<"Starting site thread";
    std::thread siteThread(&Site::run, site);

    siteThread.join();
    controllerThread.join();
}
