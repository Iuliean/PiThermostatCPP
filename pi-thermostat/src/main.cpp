#include <cstdio>
#include <fstream>
#include <expected>
#include <memory>
#include <print>

#include "nlohmann/json.hpp"

#include "spdlog/details/synchronous_factory.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "database.hpp"

struct config
{
    spdlog::level::level_enum log_level = spdlog::level::info;
    std::vector<spdlog::sink_ptr> log_sinks;
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


    if (config.contains("log_level"))
    {

        auto log_level = string_to_log_level(config.at("log_level").get<std::string>());

        if (!log_level)
            return std::unexpected{ "Failed to parse log_level" };
        out.log_level = log_level.value();
    }

    if (config.contains("log_file"))
        out.log_sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.at("log_file").get<std::string>()));

    return out;
}

void init_logging(spdlog::level::level_enum level, const std::vector<spdlog::sink_ptr>& out_sinks)
{
    auto logger = std::make_shared<spdlog::logger>(
        "pi-thermostat",
        out_sinks.begin(),
        out_sinks.end()
    );

    spdlog::set_level(level);
    spdlog::set_default_logger(logger);
}

std::expected<pi::database_manager, pi::sqlite_error> initialize_database(std::filesystem::path db_path)
{
    return ::pi::sqlite_connection::create(db_path, ::pi::sqlite_access_mode::create)
        .and_then([](pi::sqlite_connection db) -> std::expected<void, pi::sqlite_error> {
            RETURN_IF_UNEXPECTED(
                db.prepare_statement("CREATE TABLE IF NOT EXISTS Temperatures (value REAL NOT NULL, date DATE NOT NULL, time TIME NOT NULL);")
                    .and_then([](pi::sqlite_statement statement) -> std::expected<void, pi::sqlite_error> {
                        RETURN_VALUE_IF_EXPECTED_ELSE_FORWARD(statement.execute(), {});
                    }
                )
            );

            RETURN_IF_UNEXPECTED(
                db.prepare_statement("CREATE TABLE IF NOT EXISTS States (state BOOLEAN NOT NULL, date DATE NOT NULL, time TIME NOT NULL, duration INTEGER NOT NULL);")
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

    const auto config = ::load_config();
    if (!config)
    {
        std::println(stderr, "Failed to load configuration: {}", config.error());
        return 1;
    }

    ::init_logging(config->log_level, config->log_sinks);
    ::spdlog::info("Initialized logging");

    ::spdlog::info("Initializing database...");
    const auto db_init = ::initialize_database("./thermostat.db");
    if (!db_init)
    {
        ::spdlog::error("Failed to initialize database: {}", db_init.error());
        return 1;
    }

    return 0;
}