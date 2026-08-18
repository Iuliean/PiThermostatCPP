
set(SOURCE_FILES 
    src/main.cpp
)

add_executable(pi-thermostat ${SOURCE_FILES})

target_link_libraries(pi-thermostat PRIVATE crow asio)

set_property(TARGET pi-thermostat PROPERTY CXX_STANDARD 23 CXX_STANDARD_REQUIRED TRUE)

install(TARGETS pi-thermostat)