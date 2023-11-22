#include "app.h"
#include "crow/logging.h"

#include <thread>

#define LOG_APP_INFO CROW_LOG_INFO <<"[APP]:"

App::App()
    : db()
{

    ctrl = new Controller(db);
    site = new Site(db, *ctrl);
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
