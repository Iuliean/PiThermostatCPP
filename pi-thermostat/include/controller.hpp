#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP
#include "coro/mutex.hpp"
#include "database.hpp"
#include "display.hpp"
#include "nlohmann/adl_serializer.hpp"
#include "sensors/bmp280.hpp"
#include "relay.hpp"
#include "nlohmann/json.hpp"

#include <atomic>
#include <gpiod.hpp>

namespace pi
{
    class controller
    {
    public:
        struct parameters
        {
            std::int32_t min_temp;
            std::int32_t max_temp;
            std::int32_t temp_offset;
        };

        static std::expected<controller, std::string> create(database_manager& db_manager, relay relay, bmp280 temp_sensor, display& display);

        controller(database_manager& db_manager, sqlite_connection db_connection, relay relay, bmp280 temp_sensor, display& display);
        ~controller();

        coro::task<void> run();
        coro::task<void> stop() noexcept;

        coro::task<parameters> get_parameters()const noexcept;
        coro::task<void> set_parameters(std::optional<std::int32_t> min_temp = {}, std::optional<std::int32_t> max_temp = {}, std::optional<std::int32_t> temp_offset = {});

        std::int32_t get_temp()const noexcept;
    private:

        void save_parameters_to_persistance(const parameters& params)const;

        parameters m_parameters;
        mutable coro::mutex m_params_mtx;
        std::atomic_int32_t m_temp;

        coro::event m_stop;
        coro::event m_stop_flag;

        relay m_relay;
        bmp280 m_temp_sensor;
        sqlite_connection m_db_connection;
        display& m_display;
        database_manager& m_db_manager;
    };


}

namespace nlohmann
{
    template<>
    struct adl_serializer <pi::controller::parameters>
    {
        static void to_json(json& json, const pi::controller::parameters& value)
        {
            ::nlohmann::to_json(json["max_temp"], value.max_temp);
            ::nlohmann::to_json(json["min_temp"], value.min_temp);
            ::nlohmann::to_json(json["temp_offset"], value.temp_offset);
        }

        static void from_json(const json& json, pi::controller::parameters& value)
        {
            ::nlohmann::from_json(json.at("max_temp"), value.max_temp);
            ::nlohmann::from_json(json.at("min_temp"), value.min_temp);
            ::nlohmann::from_json(json.at("temp_offset"), value.temp_offset);
        }
    };
}


#endif //CONTROLLER_HPP