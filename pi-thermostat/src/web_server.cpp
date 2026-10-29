#include "web_server.hpp"
#include "controller.hpp"
#include "coro/sync_wait.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "database.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <concepts>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>


template<typename T>
    requires (!std::is_reference_v<T> && !std::is_pointer_v<T>)
static std::optional<T> get_url_param (const std::string& name, const crow::request& request)
{
    const char* value = request.url_params.get(name);
    if (!value)
        return {};

    if constexpr (std::same_as<int, T>)
        return std::stoi(value);
    if constexpr (std::same_as<long, T>)
        return std::stol(value);
    if constexpr (std::same_as<unsigned long, T>)
        return std::stoul(value);
    if constexpr (std::same_as<long long, T>)
        return std::stoll(value);
    if constexpr (std::same_as<unsigned long long, T>)
        return std::stoull(value);
    if constexpr (std::same_as<float, T>)
        return std::stof(value);
    if constexpr (std::same_as<double, T>)
        return std::stod(value);
    if constexpr (std::same_as<long double, T>)
        return std::stold(value);
    if constexpr (std::same_as<bool, T>)
        return std::stoi(value);

    std::unreachable();
}



namespace pi::web
{

    web_server::web_server(std::string address, std::uint16_t port, std::string password,database_manager* db_manager, controller* controller)
        : m_crow_app(crow::CookieParser{}, crow::SessionMiddleware<crow::InMemoryStore>{}, middleware::authorize{std::move(password)}),
          m_crow_job{},
          m_controller{controller},
          m_db_manager{db_manager},
          m_db_connection{[&db_manager](){
            auto result = db_manager->get_connection();
            if (!result)
                throw std::runtime_error(result.error().message());
            return std::move(result.value());
          }()},
          m_address{std::move(address)},
          m_port{port}
    {
        register_web_routes();
    }

    web_server::~web_server()
    {
        stop();
    }

    void web_server::run()
    {
        m_crow_job = m_crow_app
            .signal_clear()
            .bindaddr(m_address)
            .port(m_port)
            .run_async();
    }

    void web_server::stop()
    {
        m_crow_app.stop();
        if(m_crow_job.valid())
            m_crow_job.wait();
    }

    /**********
    * PRIVATE *
    **********/

    void web_server::register_web_routes()
    {
        CROW_ROUTE(m_crow_app, "/status")
            .CROW_MIDDLEWARES(m_crow_app, middleware::authorize)
            (bind_callback(&web_server::status));

        CROW_ROUTE(m_crow_app, "/set_params")
            .CROW_MIDDLEWARES(m_crow_app, middleware::authorize)
            (bind_callback(&web_server::set_params));

        CROW_ROUTE(m_crow_app, "/get_temp_history")
            .CROW_MIDDLEWARES(m_crow_app, middleware::authorize)
            (bind_callback(&web_server::get_temp_history));

        CROW_ROUTE(m_crow_app, "/get_state_history")
            .CROW_MIDDLEWARES(m_crow_app, middleware::authorize)
            (bind_callback(&web_server::get_state_history));
    }

    /****************
    * WEB CALLBACKS * 
    ****************/

    crow::response web_server::status([[maybe_unused]]const crow::request& request)
    {
        assert(m_controller);

        nlohmann::json out;
        out["temp"] = m_controller->get_temp();
        out["parameters"] = coro::sync_wait(m_controller->get_parameters());

        return crow::response{ ::crow::OK, "application/json", out.dump(4)};
    }

    crow::response web_server::set_params(const crow::request& request)
    {
        assert(m_controller);

        coro::sync_wait(
            m_controller->set_parameters(
                ::get_url_param<std::int32_t>("min_temp", request),
                ::get_url_param<std::int32_t>("max_temp", request),
                ::get_url_param<std::int32_t>("temp_offset", request)
            )
        );

        return crow::response{ ::crow::OK };
    }

    crow::response web_server::get_temp_history(const crow::request& request)
    {
        assert(m_controller);
        assert(m_db_manager);

        auto db_query = m_db_manager->get_temperature(
            m_db_connection,
            ::get_url_param<std::int64_t>("start", request).value_or(0),
            ::get_url_param<std::int64_t>("end", request).value_or(std::numeric_limits<std::int64_t>::max())
        );

        if(!db_query)
        {
            ::spdlog::error(db_query.error());
            return crow::response{ ::crow::INTERNAL_SERVER_ERROR };
        }

        return crow::response{ ::crow::OK, "application/json", db_query->dump(4) };
    }

    crow::response web_server::get_state_history(const crow::request& request)
    {
        assert(m_controller);
        assert(m_db_manager);

        auto db_query = m_db_manager->get_state(
            m_db_connection,
            ::get_url_param<std::int64_t>("start", request).value_or(0),
            ::get_url_param<std::int64_t>("end", request).value_or(std::numeric_limits<std::int64_t>::max()),
            ::get_url_param<bool>("state", request)
        );

        if(!db_query)
        {
            ::spdlog::error(db_query.error());
            return crow::response{ ::crow::INTERNAL_SERVER_ERROR };
        }

        return crow::response{ ::crow::OK, "application/json", db_query->dump(4) };
    }

}