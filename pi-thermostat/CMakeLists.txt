
set(SOURCE_FILES 
    src/main.cpp
)

add_executable(pi-thermostat ${SOURCE_FILES})

set_property(TARGET pi-thermostat PROPERTY CXX_STANDARD 23 CXX_STANDARD_REQUIRED TRUE)

install(TARGETS pi-thermostat)