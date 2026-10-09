#include "controller.hpp"

#include "coro/task.hpp"
#include "database.hpp"
#include "async/sleep.hpp"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#include <chrono>
#include <fstream>
#include <expected>
#include <filesystem>
#include <string_view>
#include <utility>

namespace pi
{
    static constexpr std::string_view g_persistence_file{"./params.json"};

    static std::int64_t get_timestamp()
    {
        return std::chrono::steady_clock::now().time_since_epoch().count();
    }

    std::expected<controller, std::string> controller::create(database_manager& db_manager, relay relay, bmp280 temp_sensor, display& display)
    {
        auto db_connection = db_manager.get_connection();

        if(!db_connection)
            return std::unexpected{
                std::format("Failed to create db connection: {}", db_connection.error())
            };

        return std::expected<controller, std::string>{
            std::in_place,
            db_manager,
            std::move(db_connection).value(),
            std::move(relay),
            temp_sensor,
            display 
        };
    }

    controller::controller(database_manager& db_manager, sqlite_connection db_connection, relay relay, bmp280 temp_sensor, display& display)
    : m_parameters{},
      m_temp{0},
      m_relay{std::move(relay)},
      m_temp_sensor{temp_sensor},
      m_db_connection{std::move(db_connection)},
      m_display{display},
      m_db_manager{db_manager}
    {
        if (!std::filesystem::exists(g_persistence_file))
        {
            ::spdlog::info("No persistence file found. Using default values");
            save_parameters_to_persistance();
            return;
        }

        std::ifstream params_file{std::filesystem::path{g_persistence_file}};

        const auto params_json = ::nlohmann::json::parse(params_file);
        const parameters parametes{
            .min_temp = params_json.at("min_temp"),
            .max_temp = params_json.at("max_temp"),
            .temp_offset = params_json.at("temp_offset")
        };

        m_parameters = parametes;
    }

    controller::~controller()
    {
        stop();
    }

    coro::task<void> controller::run()
    {
        using namespace std::chrono_literals;

        auto insert_state_to_db = [this] (bool state) {
            static auto last_state_save = std::chrono::steady_clock::now();
            const auto now = std::chrono::steady_clock::now();
            const auto res = m_db_manager.insert_state(
                m_db_connection,
                state,
                get_timestamp(),
                std::chrono::duration_cast<std::chrono::seconds>(now - last_state_save)
            );
            last_state_save = now;
            if (!res)
                ::spdlog::error("Failed to insert state into database: {}", res.error());
        };

        auto insert_temp_to_db = [this](std::int32_t temp){
            static auto last_temp_save = std::chrono::steady_clock::now();
            const auto now = std::chrono::steady_clock::now();

            if (now - last_temp_save < 1min)
                return;

            const auto res = m_db_manager.insert_temperature(
                m_db_connection,
                temp,
                get_timestamp()
            );

            if(!res)
                ::spdlog::error("Failed to insert temperatore into database: {}", res.error());
            else
                ::spdlog::debug("Temp insert");

            last_temp_save = now;
        };

        while (!m_stop)
        {
            const parameters params = m_parameters;
            const auto temp = m_temp_sensor.get_temp();

            if (temp)
            {
                m_temp = *temp + params.temp_offset;
                m_display.show(m_temp / 10);
                insert_temp_to_db(*temp);
            }
            else
                ::spdlog::error("Failed to read temperature: {}", temp.error());

            ::spdlog::debug("temp: {}, min:{}, max:{}, state:{}", m_temp.load(), params.min_temp, params.max_temp, m_relay.state());
            if (m_temp < params.min_temp && !m_relay.state())
            {
                ::spdlog::debug("Relay -> on");
                m_relay.set(true);
                insert_state_to_db(true);
            }
            else if (m_temp > params.max_temp && m_relay.state())
            {
                ::spdlog::debug("Relay -> off");
                m_relay.set(false);
                insert_state_to_db(false);
            }

            co_await async::sleep_for(100ms);
        }

        ::spdlog::debug("Controller loop exit.");
        const bool prev_value = m_stop_flag.test_and_set();
        //if the flag was already true that is very dubious
        if (prev_value)
            assert(true);
        m_stop_flag.notify_all();
    }

    void controller::stop() noexcept
    {
        m_stop = true;
        m_stop_flag.wait(false);
    }

    controller::parameters controller::get_parameters()const noexcept
    {
        return m_parameters;
    }

    void controller::set_parameters(controller::parameters value)
    {
        m_parameters = value;
        //Might write in worng order but we will get there
        save_parameters_to_persistance();
    }

    std::int32_t controller::get_temp()const noexcept
    {
        return m_temp;
    }

    /**********
    * PRIVATE *
    **********/

    void controller::save_parameters_to_persistance()const
    {
        std::ofstream out {std::filesystem::path{g_persistence_file}};
        nlohmann::json json;
        const parameters out_params = m_parameters;

        json["min_temp"] = out_params.min_temp;
        json["max_temp"] = out_params.max_temp;
        json["temp_offset"] = out_params.temp_offset;

        out << json.dump(4);
    }


}