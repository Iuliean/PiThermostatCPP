
set (HEADER_FILES 
    include/i2c/i2c_bus.hpp
    include/i2c/i2c_device.hpp

    include/sensors/bmp280.hpp

    include/async/sleep.hpp

    include/middlewares/authorize.hpp

    include/utils.hpp
    include/error.hpp
    include/events.hpp
    include/controller.hpp
    include/relay.hpp
    include/database.hpp
    include/display.hpp
    include/web_server.hpp
)

set(SOURCE_FILES 
    src/i2c/i2c_bus.cpp
    src/i2c/i2c_device.cpp

    src/sensors/bmp280.cpp

    src/async/sleep.cpp

    src/main.cpp
    src/events.cpp
    src/controller.cpp
    src/relay.cpp
    src/database.cpp
    src/display.cpp
    src/web_server.cpp
)

add_executable(pi-thermostat
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

target_link_libraries(pi-thermostat PRIVATE
    crow
    asio
    i2c-tools
    PkgConfig::gpiod
    PkgConfig::gpiodcxx
)

target_compile_options(pi-thermostat PRIVATE -Wall -Werror -Wpedantic -Wno-psabi)
target_compile_definitions(pi-thermostat PRIVATE SPDLOG_USE_STD_FORMAT)
target_link_directories(pi-thermostat PRIVATE ${gpiod_LIBRARY_DIRS})
target_include_directories(pi-thermostat PRIVATE include/)
target_link_libraries(pi-thermostat PRIVATE libcoro sqlite3 spdlog::spdlog_header_only nlohmann_json::nlohmann_json)

set_property(TARGET pi-thermostat PROPERTY CXX_STANDARD 23 CXX_STANDARD_REQUIRED TRUE CXX_SCAN_FOR_MODULES OFF)

set_target_properties(pi-thermostat PROPERTIES
    INSTALL_RPATH $ORIGIN/../lib
)

install(TARGETS pi-thermostat)