#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP
#include "controller.hpp"
#include "crow.h"
#include <spdlog/spdlog.h>
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include "database.hpp"
#include "middlewares/authorize.hpp"
#include <functional>


namespace pi::web
{
    class web_server
    {
    public:
        web_server(std::string address, std::uint16_t port, std::string password, database_manager* db_manager, controller* controller);
        ~web_server();

        void run();
        void stop();
    private:
        using crow_app = crow::Crow<
            crow::CookieParser,
            crow::SessionMiddleware<crow::InMemoryStore>,
            middleware::authorize
        >;

        void register_web_routes();

        /****************
        * WEB CALLBACKS *
        ****************/

        crow::response status(const crow::request& request);
        crow::response set_params(const crow::request& request);
        crow::response get_temp_history(const crow::request& request);
        crow::response get_state_history(const crow::request& request);

        auto bind_callback(crow::response (web_server::*fn)(const crow::request&))
        {
            return [this, fn](const crow::request& request){
                try
                {
                    return std::invoke(fn, this, request);
                }
                catch(const std::exception& e)
                {
                    ::spdlog::error("{}", e.what());
                    return crow::response{crow::INTERNAL_SERVER_ERROR};
                }
            };
        }

        crow_app m_crow_app;
        std::future<void> m_crow_job;
        controller* m_controller;
        database_manager* m_db_manager;
        sqlite_connection m_db_connection;

        std::string m_address;
        std::uint16_t m_port;

    };
}

#endif //WEB_SERVER_HPP