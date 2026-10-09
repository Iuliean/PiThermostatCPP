#include <chrono>
#include <cstdio>
#include <fstream>
#include <expected>
#include <gpiod.hpp>
#include <memory>
#include <print>
#include <string>

#include "controller.hpp"
#include "coro/sync_wait.hpp"
#include "coro/when_all.hpp"
#include "display.hpp"
#include "i2c/i2c_bus.hpp"
#include "nlohmann/json.hpp"

#include "relay.hpp"
#include "sensors/bmp280.hpp"
#include "spdlog/details/synchronous_factory.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "sensors/bmp280.hpp"
#include "database.hpp"
#include "coro/coro.hpp"
#include "async/sleep.hpp"

struct config
{
    spdlog::level::level_enum log_level = spdlog::level::info;
    std::vector<spdlog::sink_ptr> log_sinks;
    std::filesystem::path gpio_chip_path;
    std::filesystem::path i2c_bus;

    struct
    {
        std::size_t pin;
    }relay;

    struct
    {
        std::array<std::size_t, 4> digit_pins;
        std::flat_map<char, std::size_t> segment_map;
        std::size_t refresh_rate_hz;
    }display;

    struct
    {
        pi::bmp280::temperature_oversampling temp_oversampling;
        pi::bmp280::pressure_oversampling pressure_oversampling;
        pi::bmp280::power_mode power_mode;
        pi::bmp280::standby_time standby_time;
        pi::bmp280::iir_filter_coef iir_coef;
        pi::bmp280::spi_3wires spi_3wires;
    }temp_sensor;

};


constexpr std::optional<spdlog::level::level_enum> string_to_log_level(std::string_view str) noexcept
{
    if (str == "debug") return {spdlog::level::debug };
    else if (str == "info") return {spdlog::level::info };
    else if (str == "warn") return {spdlog::level::warn };
    else if (str == "err") return {spdlog::level::err };
    else return { };
}

std::expected<config, std::string> load_config()
{
    config out;
    std::ifstream config_file{"conf.json"};
    const auto config = nlohmann::json::parse(config_file);

    out.log_sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    auto get_or = [](const nlohmann::json& json, const std::string& key, auto value)
    {
        try
        {
            return json.at(key).get<decltype(value)>();
        }
        catch(...)
        {
            return value;
        }
    };


    if (config.contains("log_level"))
    {

        auto log_level = string_to_log_level(config.at("log_level").get<std::string>());

        if (!log_level)
            return std::unexpected{ "Failed to parse log_level" };
        out.log_level = log_level.value();
    }
    else
        out.log_level = spdlog::level::info;

    if (config.contains("log_file"))
        out.log_sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.at("log_file").get<std::string>()));

    if (config.contains("gpio_chip"))
        out.gpio_chip_path = config.at("gpio_chip").get<std::string>();
    else
        return std::unexpected{"\"gpio_chip\" field is required"};

    if (config.contains("i2c_bus"))
        out.i2c_bus = config.at("i2c_bus").get<std::string>();
    else
        return std::unexpected{"\"i2c_bus\" field is required"};



    /*------------------------------------------------------------------------------------*/


    if (!config.contains("relay"))
        return std::unexpected{"\"relay\" field is required"};

    auto relay = config.at("relay");
    if (!relay.contains("pin"))
        return std::unexpected{"\"relay.pin\" filed is required"};

    out.relay.pin = relay.at("pin").get<std::size_t>();


    /*------------------------------------------------------------------------------------*/


    if (config.contains("display"))
    {
        auto display = config["display"];
        if (!display.contains("digit_pins") || !display.contains("segment_map"))
            return std::unexpected("\"digit_pins\" and \"segment_map\" fields are required");

        out.display.digit_pins = display.at("digit_pins").get<decltype(out.display.digit_pins)>();
        for (auto [segment, pin] : display.at("segment_map").items())
        {
            out.display.segment_map[segment[0]] = pin.get<std::size_t>();
        }

        out.display.refresh_rate_hz =
            display.contains("refresh_rate.hz") ? display.at("refresh_rate_hz").get<std::size_t>() : 60;

    }
    else
        return std::unexpected("\"display\" field is required");


    /*------------------------------------------------------------------------------------*/

    if(config.contains("temp_sensor"))
    {
        auto temp_sensor = config.at("temp_sensor");
        out.temp_sensor.temp_oversampling = *::pi::bmp280::temp_oversampling_from(get_or(temp_sensor, "temp_oversampling", 8));
        out.temp_sensor.pressure_oversampling = *::pi::bmp280::pressure_oversampling_from(get_or(temp_sensor, "pressure_oversampling", 0));
        out.temp_sensor.power_mode = *::pi::bmp280::power_mode_from(get_or(temp_sensor, "power_mode", std::string{"normal"}));
        out.temp_sensor.standby_time = *::pi::bmp280::standby_time_from(get_or(temp_sensor, "standby_time", 4000));
        out.temp_sensor.iir_coef = *::pi::bmp280::iir_coef_from(get_or(temp_sensor, "iir_coef", 2));
        out.temp_sensor.spi_3wires = pi::bmp280::spi_3wires{get_or(temp_sensor, "spi_3wires", 0)};
    }
    else
    {
        out.temp_sensor.temp_oversampling = ::pi::bmp280::temperature_oversampling::x4;
        out.temp_sensor.pressure_oversampling = ::pi::bmp280::pressure_oversampling::disable;
        out.temp_sensor.power_mode = ::pi::bmp280::power_mode::normal;
        out.temp_sensor.standby_time = ::pi::bmp280::standby_time::ms_4000;
        out.temp_sensor.iir_coef = ::pi::bmp280::iir_filter_coef::x2;
        out.temp_sensor.spi_3wires = ::pi::bmp280::spi_3wires::disable;
    }

    /*------------------------------------------------------------------------------------*/

    return out;
}

void init_logging(spdlog::level::level_enum level, const std::vector<spdlog::sink_ptr>& out_sinks)
{
    auto logger = std::make_shared<spdlog::logger>(
        "pi-thermostat",
        out_sinks.begin(),
        out_sinks.end()
    );

    spdlog::set_default_logger(logger);
    spdlog::set_level(level);
}

std::expected<pi::database_manager, pi::sqlite_error> initialize_database(std::filesystem::path db_path)
{
    return ::pi::sqlite_connection::create(db_path, ::pi::sqlite_access_mode::create)
        .and_then([](pi::sqlite_connection db) -> std::expected<void, pi::sqlite_error> {
            RETURN_IF_UNEXPECTED(
                db.prepare_statement("CREATE TABLE IF NOT EXISTS temperatures (value INTEGER NOT NULL, timestamp INTEGER NOT NULL);")
                    .and_then([](pi::sqlite_statement statement) -> std::expected<void, pi::sqlite_error> {
                        RETURN_VALUE_IF_EXPECTED_ELSE_FORWARD(statement.execute(), {});
                    }
                )
            );

            RETURN_IF_UNEXPECTED(
                db.prepare_statement("CREATE TABLE IF NOT EXISTS states (state BOOLEAN NOT NULL, timestamp INTEGER NOT NULL, duration INTEGER NOT NULL);")
                    .and_then([](pi::sqlite_statement statement) -> std::expected<void, pi::sqlite_error> {
                        RETURN_VALUE_IF_EXPECTED_ELSE_FORWARD(statement.execute(), {});
                    }
                )
            );
            return {};
        })
        .transform([&db_path]() -> pi::database_manager { return pi::database_manager{ std::move(db_path) }; });
}

int main()
{
    using namespace std::chrono_literals; 

    const auto config = ::load_config();
    if (!config)
    {
        std::println(stderr, "Failed to load configuration: {}", config.error());
        return 1;
    }

    ::init_logging(config->log_level, config->log_sinks);
    ::spdlog::info("Initialized logging");

    ::spdlog::info("Initializing database...");
    auto db_init = ::initialize_database("./thermostat.db");
    if (!db_init)
    {
        ::spdlog::error("Failed to initialize database: {}", db_init.error());
        return 1;
    }
    ::spdlog::info("Initializing database... Done");

    ::spdlog::info("Initializing I2C bus...");
    auto i2c_bus = ::i2c::bus::open(config->i2c_bus);
    if (!i2c_bus)
    {
        ::spdlog::error("Failed to initialize the i2c_bus: {}", i2c_bus.error());
        return 1;
    }
    ::spdlog::info("Initializing I2C bus... Done");

    ::spdlog::info("Initializing bmp280 sensor...");
    auto temp_sensor = ::pi::bmp280::create(
        i2c_bus.value(),
        pi::bmp280::config{}
        .set_iir_filter_coef(config->temp_sensor.iir_coef)
        .set_temperature_oversampling(config->temp_sensor.temp_oversampling)
        .set_pressure_oversampling(config->temp_sensor.pressure_oversampling)
        .set_power_mode(config->temp_sensor.power_mode)
        .set_standby_time(config->temp_sensor.standby_time)
        .set_spi_3wires(config->temp_sensor.spi_3wires)
    );
    if (!temp_sensor)
    {
        ::spdlog::error("Failed to initialize temp sensor: {}", temp_sensor.error());
        return 1;
    }
    ::spdlog::info("Initializing bmp280 sensor... Done");
    
    gpiod::chip gpio_chip{config->gpio_chip_path};

    pi::display display{
        gpio_chip,
        config->display.segment_map,
        config->display.digit_pins
    };

    pi::relay relay{gpio_chip, config->relay.pin};

    ::spdlog::info("Initializing controller...");
    auto controller = ::pi::controller::create(
        db_init.value(),
        std::move(relay),
        temp_sensor.value(),
        display
    );
    if(!controller)
    {
        ::spdlog::error("Failed to initialize controller", controller.error());
        return 1;
    }
    ::spdlog::info("Initializing controller... Done");

    coro::sync_wait(coro::when_all(display.run(), controller->run()));
    return 0;
}