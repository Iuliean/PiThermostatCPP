
set(SOURCE_FILES 
    src/main.cpp
)

add_executable(pi-thermostat ${SOURCE_FILES})

target_link_libraries(pi-thermostat PRIVATE
    crow
    asio
    PkgConfig::gpiod
    PkgConfig::gpiodcxx
)

target_link_directories(pi-thermostat PRIVATE ${gpiod_LIBRARY_DIRS})

set_property(TARGET pi-thermostat PROPERTY CXX_STANDARD 23 CXX_STANDARD_REQUIRED TRUE)

set_target_properties(pi-thermostat PROPERTIES
    INSTALL_RPATH $ORIGIN/../lib
)
install(TARGETS pi-thermostat)