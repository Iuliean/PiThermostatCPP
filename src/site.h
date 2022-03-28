#pragma once
#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "controller.h"
#include "file.h"
#include "database.h"
#include "json_custom.h"
#include "cookie.h"

#include <string>

struct Authentificator : crow::ILocalMiddleware
{
    struct context{};

    void before_handle(crow::request& req, crow::response& res, context& ctx)
    {
        CROW_LOG_DEBUG << "keek";
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx)
    {
        CROW_LOG_DEBUG << "lol";
    }
};

class Site
{
private:
    DataBase&       db = DataBase::getInstance();
    File            configFile{"config.json"};
    Controller*     cntrl;

    int             port;
    unsigned int    cleanInterval;
    
    std::string     password;

    crow::App<crow::CookieParser, Authentificator> app;
public:
    Site(Controller* otherController);

    void run();


private:
    void auth(const crow::request& req, crow::response& resp);
    
    void loginPage(crow::response& resp);
    void dashboard(crow::response& resp);
    
    void getParams(crow::response& resp);
    void setParams(crow::response& resp, const json& newSettings);

    void getTemps(crow::response& resp, const std::string& start = "24h", const std::string& end = "now");
    void getAverage(crow::response& resp);
    void getAveragePast24h(crow::response& resp);
    void getAverage(crow::response& resp, const std::string& start, const std::string& end = "now");

    void getStates(crow::response& resp, const std::string& state, const std::string& start = "24h", const std::string& end = "now");
    void getAverageOnTime(crow::response& resp);
    void getAverageOnTimePast24h(crow::response& resp);
    void getAverageOnTime(crow::response& resp, const std::string& start, const std::string& end = "now");
    
    void shutdown(crow::response& resp);
    
};