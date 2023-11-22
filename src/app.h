#pragma once
#include "site.h"
#include "controller.h"
#include "database.h"


class App
{
private:
    Controller* ctrl;
    Site* site;
    DataBase db;
public:

    App();
    ~App();

    void run();
};