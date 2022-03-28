#pragma once
#include "site.h"
#include "controller.h"


class App
{
private:
    Controller* ctrl;
    Site* site;
public:

    App();
    ~App();

    void run();
};