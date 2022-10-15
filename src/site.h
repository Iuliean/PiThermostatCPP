#pragma once
#include "crow.h"
#include "crow/logging.h"
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
        if(Cookie::verifyCookie(req.get_header_value("Authorization")))
        {
            if(req.url == "/")
            {
                res.redirect("/dashboard");
                res.end();
            }
            else
                return;
        }
        else
        {
            if(req.url == "/")
                return;
            else
            {
                res.code = 401;
                res.end();
                return;
            }
        }
        
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx){}
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
    
    void loginPage(crow::request& req, crow::response& resp);
    void dashboard(crow::request& req, crow::response& resp);
    
    void getParams(crow::request& req, crow::response& resp);
    void setParams(crow::request& req, crow::response& resp);

    void getTemps(crow::request& req, crow::response& resp);
    void getAverage(crow::request& req, crow::response& resp);
    
    void getStates(crow::request& req, crow::response& resp);
    void getAverageStateTime(crow::request& req, crow::response& resp);
    
    void shutdown(crow::request& req, crow::response& resp);
    
};