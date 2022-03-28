#include "site.h"
#include "sha/sha256.h"

#include <thread>
#include <stdio.h>

Site::Site(Controller* otherController)
{
    this->cntrl         = otherController;

    class SHA256 hasher;
    json j    = configFile.read();
    this->port          = j["site"]["port"];
    this->cleanInterval = j["site"]["cleanInterval"];
    this->password      = hasher(j["site"]["password"]);
    Cookie::lifetime    = j["site"]["cookieLifetime"];

    app.loglevel(crow::LogLevel::Info);

    CROW_ROUTE(this->app, "/").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.redirect("/dashboard");
                                    resp.end();
                                    return;
                                }
                                this->loginPage(resp);
                            });
    
    CROW_ROUTE(this->app, "/auth").methods(crow::HTTPMethod::POST)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                this->auth(req, resp);
                            });
    
    CROW_ROUTE(this->app, "/dashboard").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 303;
                                    resp.set_header("location", "/");
                                    resp.end();     
                                    return;
                                }
                                this->dashboard(resp);
                            });
    
    CROW_ROUTE(this->app, "/getParams").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getParams(resp);
                            });

    CROW_ROUTE(this->app, "/setParams").methods(crow::HTTPMethod::POST)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                json newSettings = json::parse(req.body);
                                this->setParams(resp, newSettings);
                            });
                            
    CROW_ROUTE(this->app, "/temperature/get/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp, const std::string& start)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getTemps(resp, start);
                            });
    
    CROW_ROUTE(this->app, "/temperature/get/<string>/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp, const std::string& start, const std::string& end)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getTemps(resp, start, end);
                            });
                            
    CROW_ROUTE(this->app, "/temperature/get/24h").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getTemps(resp);
                            });
    
    CROW_ROUTE(this->app, "/temperature/getAverage").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAverage(resp);
                            });

    CROW_ROUTE(this->app, "/temperature/getAverage/24h").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAveragePast24h(resp);
                            });
    
    CROW_ROUTE(this->app, "/temperature/getAverage/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp,const std::string& start)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAverage(resp, start);
                            });

    CROW_ROUTE(this->app, "/temperature/getAverage/<string>/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp,const std::string& start, const std::string& end)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAverage(resp, start, end);
                            });
    
    CROW_ROUTE(this->app, "/state/get/<string>/24h").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp, const std::string& state)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getStates(resp, state);
                            });

    CROW_ROUTE(this->app, "/state/get/<string>/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp, const std::string& state, const std::string& start)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getStates(resp, state, start);
                            }); 

    CROW_ROUTE(this->app, "/state/get/<string>/<string>/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp, const std::string& state, const std::string& start, const std::string& end)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getStates(resp, state ,start, end);
                            });

    CROW_ROUTE(this->app, "/state/getAverageOnTime").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAverageOnTime(resp);
                            });

    CROW_ROUTE(this->app, "/state/getAverageOnTime/24h").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAverageOnTimePast24h(resp);
                            });

    CROW_ROUTE(this->app, "/state/getAverageOnTime/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp,const std::string& start)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAverageOnTime(resp, start);
                            });
            
    CROW_ROUTE(this->app, "/state/getAverageOnTime/<string>/<string>").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp,const std::string& start, const std::string& end)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                this->getAverageOnTime(resp, start, end);
                            }); 

    CROW_ROUTE(this->app, "/shutdown").methods(crow::HTTPMethod::GET)
                            ([this](const crow::request req, crow::response& resp)
                            {
                                auto& cookies = this->app.get_context<crow::CookieParser>(req);
                                if(!Cookie::verifyCookie(cookies.get_cookie("authToken")))
                                {
                                    resp.code = 401;
                                    resp.body = "Unauthorized access";
                                    resp.end();
                                    return;
                                }
                                return this->shutdown(resp);
                            });
}

void Site::run()
{
    std::thread cookieCleaner([this](){
        while(true)
        {
            CROW_LOG_INFO << "Cookie cleaner sleeping for " << this->cleanInterval << "secs";
            std::this_thread::sleep_for(std::chrono::seconds(this->cleanInterval));
            CROW_LOG_INFO << "Cleaning cookies..."; 
            Cookie::cookieCleaner();
        }
    });
    app.signal_clear();
    app.port(this->port).multithreaded().run();
}

//Private

void Site::auth(const crow::request& req, crow::response& resp)
{
    CROW_LOG_INFO << "Login Attempt";
    crow::multipart::message msg(req);
    if(msg.parts[0].body == this->password)
    {
        resp.code = 303;
        resp.set_header("location", "/dashboard");
        resp.add_header("set-cookie", Cookie::generateCookie().toString());
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

void Site::loginPage(crow::response& resp)
{
    CROW_LOG_INFO << "Serving login page";
    resp.code = 200;
    resp.body = crow::mustache::load("login.html").render_string();
    resp.end();
}


void Site::dashboard(crow::response& resp)
{
    CROW_LOG_INFO << "Serving Dashboard page";

    crow::mustache::context pageContext;
    const Parameters params     = this->cntrl->getParameters();
    
    pageContext["minTemp"]      = params.minTemp;
    pageContext["maxTemp"]      = params.maxTemp;
    pageContext["temp"]         = params.temp;
    pageContext["state"]        = params.state ? "ON" : "OFF";

    resp.code = 200;
    resp.body = crow::mustache::load("dashboard.html").render_string(pageContext);
    resp.end();
}


void Site::getParams(crow::response& resp)
{
    json returnJson;
    const Parameters params     = this->cntrl->getParameters();

    returnJson["minTemp"]       = params.minTemp;
    returnJson["maxTemp"]       = params.maxTemp;
    returnJson["temp"]          = params.temp;
    returnJson["state"]         = params.state ? "ON" : "OFF";

    
    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::setParams(crow::response& resp, const json& newSettings)
{
    CROW_LOG_INFO << "SetParams call";
    if(newSettings["minTemp"] != nullptr && newSettings["maxTemp"] != nullptr)
        this->cntrl->setParameters(newSettings["minTemp"], newSettings["maxTemp"]);
    else
    {
        if(newSettings["minTemp"] == nullptr)
            this->cntrl->setMaxTemp(newSettings["maxTemp"]);
        else
            this->cntrl->setMinTemp(newSettings["minTemp"]);
    }

    resp.code = 200;
    resp.end();
}

void Site::getTemps(crow::response& resp, const std::string& start, const std::string& end)
{
    json returnJson;
    
    if(start == "24h")
        this->db.getTemperaturesPast24h(returnJson);
    else
        this->db.getTemperatures(start, end, returnJson);
    
    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAverage(crow::response& resp)
{
    json returnJson;

    this->db.getAverageTemp(returnJson);

    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAveragePast24h(crow::response& resp)
{
    json returnJson;

    this->db.getAverageTempPast24h(returnJson);

    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAverage(crow::response& resp, const std::string& start, const std::string& end)
{
    json returnJson;
    
    this->db.getAverageTemp(start, end, returnJson);

    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getStates(crow::response& resp, const std::string& state, const std::string& start, const std::string& end)
{
    json returnJson;

    if(state == "off")
    {
        if(start == "24h")
            this->db.getStates24h("0", returnJson);
        else
            this->db.getStates("0", start, end, returnJson);
    }
    else
    {
        if(start == "24h")
            this->db.getStates24h("1", returnJson);
        else
            this->db.getStates("1", start, end, returnJson);
    }

    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAverageOnTime(crow::response& resp)
{
    json returnJson;
    
    this->db.getAverageOnTime(returnJson);
    
    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAverageOnTimePast24h(crow::response& resp)
{
    json returnJson;
    
    this->db.getAverageOnTimePast24h(returnJson);
  
    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::getAverageOnTime(crow::response& resp, const std::string& start, const std::string& end)
{
    json returnJson;
    
    this->db.getAverageOnTime(start, end, returnJson);

    resp.body = returnJson.dump(4);
    resp.code = 200;
    resp.set_header("Content-Type", "Application/json");
    resp.end();
}

void Site::shutdown(crow::response& resp)
{
    CROW_LOG_INFO << "Shutting down";

    this->cntrl->toDisk();
    system("sudo shutdown +1");

    resp.code = 200;
    resp.end();
}



