#include "site.h"
#include "json/json.hpp"
#include "sha/sha256.h"
#include "cookie.h"

#include <thread>

Site::Site(Controller* otherController)
{
    this->cntrl = otherController;

    SHA256 hasher;
    nlohmann::json j = configFile.read();

    this->port          = j["site"]["port"];
    Cookie::lifetime    = j["site"]["cookieLifetime"];
    this->password      = hasher(j["site"]["password"]);


    app.loglevel(crow::LogLevel::Info);
    
    
    CROW_ROUTE(this->app, "/").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                return this->loginPage(req, cookies);
                            });
    
    CROW_ROUTE(this->app, "/auth").methods(crow::HTTPMethod::POST)
                            ([this](const crow::request req)
                            {
                                return this->auth(req);
                            });
    
    CROW_ROUTE(this->app, "/dashboard").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                return this->dashboard(req, cookies);
                            });
    
    CROW_ROUTE(this->app, "/getParams").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                return this->getParams(cookies);
                            });
        
    CROW_ROUTE(this->app, "/setParams").methods(crow::HTTPMethod::POST)
                            ([this](const crow::request req)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                return this->setParams(req,cookies);
                            });
}

void Site::run()
{
    std::thread cookieCleaner([](){
        while(true)
        {
            CROW_LOG_INFO << "Cookie cleaner sleeping for 1min";
            std::this_thread::sleep_for(std::chrono::minutes(1));
            CROW_LOG_INFO << "Cleaning cookies..."; 
            Cookie::cookieCleaner();
        }
    });
    app.signal_clear();
    app.port(this->port).multithreaded().run();
}

//Private

crow::response Site::auth(const crow::request& req)
{
    CROW_LOG_INFO << "Login Attempt";
    crow::multipart::message msg(req);
    if(msg.parts[0].body == this->password)
    {
        crow::response resp(303);
        resp.set_header("location", "/dashboard");
        resp.add_header("set-cookie", Cookie::generateCookie().toString());
        return resp;
    }
    else
    {
        return crow::response(401, "Failed login attempt");
    }
}

crow::response Site::loginPage(const crow::request& req, const crow::CookieParser::context& cookies)
{
    CROW_LOG_INFO << "Serving login page";
    if(Cookie::verifyCookie(cookies.get_cookie("authToken")))
    {
        crow::response resp;
        resp.redirect("/dashboard");
        return resp;
    }
    else
    {
        return crow::response(200, crow::mustache::load("login.html").render());
    }
}


crow::response Site::dashboard(const crow::request& req, const crow::CookieParser::context& cookies)
{
    CROW_LOG_INFO << "Serving Dashboard page";
    
    if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
    {
        crow::response resp (303);
        resp.set_header("location", "/");     
        return resp;
    }
    
    crow::mustache::context pageContext;
    const Parameters params     = this->cntrl->getParameters();
    
    pageContext["threshold"]    = params.threshold;
    pageContext["range"]        = params.range;
    pageContext["temp"]         = params.temp;
    pageContext["state"]        = params.state ? "ON" : "OFF";
    return crow::response(200, crow::mustache::load("dashboard.html").render(pageContext));
}


crow::response Site::getParams(const crow::CookieParser::context& cookies)
{
    if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
    {
        return crow::response(401, "Unauthorized access");
    }

    crow::json::wvalue returnJson;
    const Parameters params     = this->cntrl->getParameters();

    returnJson["threshold"]     = params.threshold;
    returnJson["range"]         = params.range;
    returnJson["temp"]          = params.temp;
    returnJson["state"]         = params.state ? "ON" : "OFF";

    return crow::response(200, returnJson);
}

crow::response Site::setParams(const crow::request& req, const crow::CookieParser::context& cookies)
{
    CROW_LOG_INFO << "SetParams call";
    if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
    {
        return crow::response(401, "Unauthorized access");
    }

    nlohmann::json newSettings = nlohmann::json::parse(req.body);

    if(newSettings["threshold"] != nullptr && newSettings["range"] != nullptr)
        this->cntrl->setParameters(newSettings["threshold"], newSettings["range"]);
    else
    {
        if(newSettings["threshold"] == nullptr)
            this->cntrl->setRange(newSettings["range"]);
        else
            this->cntrl->setThreshold(newSettings["threshold"]);
    }

    return crow::response(200);
}




