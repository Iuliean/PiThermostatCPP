#pragma once
#include "crow/crow_all.h"
#include "controller.h"
#include "file.h"


class Site
{
private:
    File configFile{"config.json"};
    Controller* cntrl;

    crow::App<crow::CookieParser> app;

    int port;
    unsigned int cleanInterval;
    std::string password;

public:
    Site(Controller* otherController);

    void run();


private:
    crow::response auth(const crow::request& req);
    
    crow::response loginPage(const crow::request& req, const crow::CookieParser::context& cookies);
    crow::response dashboard(const crow::request& req, const crow::CookieParser::context& cookies);
    
    crow::response getParams(const crow::CookieParser::context& cookies);
    crow::response setParams(const crow::request& req, const crow::CookieParser::context& cookies);
    
    crow::response shutdown(const crow::request& req, const crow::CookieParser::context& cookies);
    
};