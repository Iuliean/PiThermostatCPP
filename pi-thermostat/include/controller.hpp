#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP
#include "database.hpp"
#include "display.hpp"
#include "sensors/bmp280.hpp"
#include "relay.hpp"

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
        void stop() noexcept;

        parameters get_parameters()const noexcept;
        void set_parameters(parameters value);

        std::int32_t get_temp()const noexcept;
    private:

        void save_parameters_to_persistance()const;

        std::atomic<parameters> m_parameters;
        std::atomic_int32_t m_temp;

        std::atomic_bool m_stop;
        std::atomic_flag m_stop_flag;

        relay m_relay;
        bmp280 m_temp_sensor;
        sqlite_connection m_db_connection;
        display& m_display;
        database_manager& m_db_manager;
    };
}

#endif //CONTROLLER_HPP