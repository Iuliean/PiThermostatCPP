#include "site.h"
#include "app.h"

#include "sha/sha256.h"
 
#include <string>
#include <thread>
#include <stdio.h>
#define ROUTE(url, method, func) CROW_ROUTE(app, url)\
                                .methods(method)\
                                ([this](crow::request& req, crow::response& res){func(req,res);})

#define ROUTE_MIDDLEWARES(url, method, middleware, func) CROW_ROUTE(app, url)\
                                                        .methods(method)\
                                                        .CROW_MIDDLEWARES(app, middleware)\
                                                        ([this](crow::request& req, crow::response& res){func(req,res);})

Site::Site(DataBase& db, Controller& controller)
    :db(db),
    cntrl(controller),
    configFile("config.json")
{
    
    class SHA256 hasher;
    json j              = configFile.read();
    port                = j["site"]["port"];
    cleanInterval       = j["site"]["cleanInterval"];
    password            = hasher(j["site"]["password"]);
    Cookie::lifetime    = j["site"]["cookieLifetime"];

    app.loglevel(crow::LogLevel::Info);
#ifdef CROW_ENABLE_SSL
    if(j["site"]["ssl_files"][0] != nullptr && j["site"]["ssl_files"][1] != nullptr)
    {
        app.ssl_file(j["site"]["ssl_files"][0], j["site"]["ssl_files"][1]);
    }
#endif
    ROUTE_MIDDLEWARES("/", crow::HTTPMethod::GET, Authentificator, loginPage);

    ROUTE("/auth", crow::HTTPMethod::POST, auth);

    ROUTE_MIDDLEWARES("/dashboard", crow::HTTPMethod::GET, Authentificator, dashboard);
    
    ROUTE_MIDDLEWARES("/getParams", crow::HTTPMethod::GET, Authentificator, getParams);

    ROUTE_MIDDLEWARES("/setParams", crow::HTTPMethod::POST, Authentificator, setParams);
                            
    ROUTE_MIDDLEWARES("/temperature/get", crow::HTTPMethod::GET, Authentificator, getTemps);
    
    ROUTE_MIDDLEWARES("/temperature/getAverage", crow::HTTPMethod::GET, Authentificator, getAverage);

    ROUTE_MIDDLEWARES("/state/get", crow::HTTPMethod::GET, Authentificator, getStates);

    ROUTE_MIDDLEWARES("/state/getAverage", crow::HTTPMethod::GET, Authentificator, getAverageStateTime);

    ROUTE_MIDDLEWARES("/shutdown", crow::HTTPMethod::GET, Authentificator, shutdown);
}

void Site::run()
{
    std::thread cookieCleaner([this](){
        while(true)
        {
            CROW_LOG_INFO << "Cookie cleaner sleeping for " << cleanInterval << "secs";
            std::this_thread::sleep_for(std::chrono::seconds(cleanInterval));
            CROW_LOG_INFO << "Cleaning cookies..."; 
            Cookie::cookieCleaner();
        }
    });
    app.signal_clear();
    app.port(port).multithreaded().run();
}

//Private

void Site::auth(const crow::request& req, crow::response& resp)
{
    CROW_LOG_INFO << "Login Attempt";
    crow::multipart::message msg(req);
    if(msg.parts[0].body == password)
    {
        resp.body = "{\n\t\"token\": \"" + Cookie::generateCookie().lock()->token() + "\"\n}";
        resp.end();
        return;
    }
    else
    {
        resp.code = 401;
        resp.body = "Failed login attempt";
        resp.end();
        return;
    }
}

void Site::loginPage(crow::request& req, crow::response& resp)
{
    CROW_LOG_INFO << "Serving login page";
    resp.code = 200;
    resp.body = crow::mustache::load("login.html").render_string();
    resp.end();
}


void Site::dashboard(crow::request& req, crow::response& resp)
{
    CROW_LOG_INFO << "Serving Dashboard page";

    crow::mustache::context pageContext;
    const Parameters params     = cntrl.getParameters();
    
    pageContext["minTemp"]      = params.minTemp;
    pageContext["maxTemp"]      = params.maxTemp;
    pageContext["temp"]         = params.temp;
    pageContext["state"]        = params.state ? "ON" : "OFF";

    resp.code = 200;
    resp.body = crow::mustache::load("dashboard.html").render_string(pageContext);
    resp.end();
}


void Site::getParams(crow::request& req, crow::response& resp)
{
    json returnJson;
    const Parameters params     = cntrl.getParameters();
    
    returnJson["status"]                = 0;
    returnJson["data"]["minTemp"]       = params.minTemp;
    returnJson["data"]["maxTemp"]       = params.maxTemp;
    returnJson["data"]["temp"]          = params.temp;
    returnJson["data"]["state"]         = params.state ? "ON" : "OFF";

    resp.body = returnJson.dump(4); 
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::setParams(crow::request& req, crow::response& resp)
{
    const json newSettings = json::parse(req.body);
    if(newSettings["minTemp"] != nullptr && newSettings["maxTemp"] != nullptr)
        cntrl.setParameters(newSettings["minTemp"], newSettings["maxTemp"]);
    else
    {
        if(newSettings["minTemp"] == nullptr)
            cntrl.setMaxTemp(newSettings["maxTemp"]);
        else
            cntrl.setMinTemp(newSettings["minTemp"]);
    }

    resp.code = 200;
    resp.end();
}

void Site::getTemps(crow::request& req, crow::response& resp)
{
    json returnJson;
    returnJson["data"] = json::array();
    
    char* t1= req.url_params.get("startDate");
    char* t2= req.url_params.get("endDate");

    std::string start(t1 ? t1 : "");
    std::string end(t2 ? t2 : "");

    
    if(start.empty()&& end.empty())
        db.getTemperaturesPast24h(returnJson);
    else
    {
        if(start.empty())
            db.getTemperatures("1970-01-01", end, returnJson);
        else
        {
            if(start == "24h")
                db.getTemperaturesPast24h(returnJson);
            else
            {
                if(end.empty())
                    db.getTemperatures(start, "now" ,returnJson);
                else
                    db.getTemperatures(start, end, returnJson);
            }
        }
    }
    resp.body           = returnJson.dump(4);
    resp.code           = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAverage(crow::request& req, crow::response& resp)
{
    json returnJson;
    returnJson["data"] = json::array();

    char* t1= req.url_params.get("startDate");
    char* t2= req.url_params.get("endDate");

    std::string start(t1 ? t1 : "");    
    std::string end(t2 ? t2 : "");
    if(start.empty()  && end.empty())
        db.getAverageTemp(returnJson);
    else
    {
        if(start.empty())
            db.getAverageTemp("1970-01-01", end, returnJson);
        else
        {
            if(start == "24h")
                db.getAverageTempPast24h(returnJson);
            else
            {
                if(end.empty())
                    db.getAverageTemp(start, "now" ,returnJson);
                else
                    db.getAverageTemp(start, end, returnJson);
            }
        }
    }    
    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getStates(crow::request& req, crow::response& resp)
{
    json returnJson;
    returnJson["data"] = json::array();

    char* t1= req.url_params.get("state");
    char* t2= req.url_params.get("startDate");
    char* t3= req.url_params.get("endDate");

    std::string state(t1 ? t1 : "on");
    std::string start(t2 ? t2 : "");
    std::string end(t3 ? t3 : "");

    if(state == "on")
    {
       if(start.empty())
       {
            if(end.empty())
                db.getStates24h("1", returnJson);
            else
                db.getStates("1", "1970-01-01", end, returnJson);
       }
       else
       {
            if(start == "24h")
                db.getStates24h("1", returnJson);
            else
            {
                if(end.empty())
                    db.getStates("1", start, "now", returnJson);
                else
                    db.getStates("1", start, end, returnJson);
            }
       }
    }
    else
    {
       if(start.empty())
       {
            if(end.empty())
                db.getStates24h("0", returnJson);
            else
                db.getStates("0", "1970-01-01", end, returnJson);
       }
       else
       {
            if(start == "24h")
                db.getStates24h("0", returnJson);
            else
            {
                if(end.empty())
                    db.getStates("0", start, "now", returnJson);
                else
                    db.getStates("0", start, end, returnJson);
            }
       }
    }
    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAverageStateTime(crow::request& req, crow::response& resp)
{
    json returnJson;
    returnJson["data"] = json::array();
    
    char* t1= req.url_params.get("state");
    char* t2= req.url_params.get("startDate");
    char* t3= req.url_params.get("endDate");

    std::string state(t1 ? t1 : "on");
    std::string start(t2 ? t2 : "");
    std::string end(t3 ? t3 : "");

    if(state == "on")
    {
       if(start.empty())
       {
            if(end.empty())
                db.getAverageStateTimePast24h("1", returnJson);
            else
                db.getAverageStateTime("1", "1970-01-01", end, returnJson);
       }
       else
       {
            if(start == "24h")
                db.getAverageStateTimePast24h("1", returnJson);
            else
            {
                if(end.empty())
                    db.getAverageStateTime("1", start, "now", returnJson);
                else
                    db.getAverageStateTime("1", start, end, returnJson);
            }
       }
    }
    else
    {
       if(start.empty())
       {
            if(end.empty())
                db.getAverageStateTimePast24h("0", returnJson);
            else
                db.getAverageStateTime("0", "1970-01-01", end, returnJson);
       }
       else
       {
            if(start == "24h")
                db.getAverageStateTimePast24h("0", returnJson);
            else
            {
                if(end.empty())
                    db.getAverageStateTime("0", start, "now", returnJson);
                else
                    db.getAverageStateTime("0", start, end, returnJson);
            }
       }
    }
    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}


void Site::shutdown(crow::request& req, crow::response& resp)
{
    CROW_LOG_INFO << "Shutting down";

    cntrl.toDisk();
    system("sudo shutdown +1");

    resp.code = 200;
    resp.end();
}   



