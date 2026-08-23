
set (HEADER_FILES 
    include/i2c/i2c_bus.hpp
    include/i2c/i2c_device.hpp

    include/sensors/bmp280.hpp

    include/error.hpp
)

set(SOURCE_FILES 
    src/i2c/i2c_bus.cpp
    src/i2c/i2c_device.cpp

    src/sensors/bmp280.cpp

    src/main.cpp
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

target_compile_options(pi-thermostat PRIVATE -Wall -Werror -Wpedantic)
target_link_directories(pi-thermostat PRIVATE ${gpiod_LIBRARY_DIRS})
target_include_directories(pi-thermostat PRIVATE include/)
target_link_libraries(pi-thermostat PRIVATE libcoro)

set_property(TARGET pi-thermostat PROPERTY CXX_STANDARD 23 CXX_STANDARD_REQUIRED TRUE)

set_target_properties(pi-thermostat PROPERTIES
    INSTALL_RPATH $ORIGIN/../lib
)
install(TARGETS pi-thermostat)