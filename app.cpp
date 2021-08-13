#include "app.h"
#include "crow/crow_all.h"

#include <thread>

#define LOG_APP_INFO CROW_LOG_INFO <<"[APP]:"

App::App()
{

    this->ctrl = new Controller;
    this->site = new Site(this->ctrl);
}

App::~App()
{
    delete this->site;
    delete this->ctrl;
}

void App::run()
{  
    LOG_APP_INFO << "Starting controller thread";
    std::thread controllerThread(&Controller::run, this->ctrl);

    LOG_APP_INFO <<"Starting site thread";
    std::thread siteThread(&Site::run, this->site);

    siteThread.join();
    controllerThread.join();
}
