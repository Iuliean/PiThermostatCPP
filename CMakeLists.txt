cmake_minimum_required(VERSION 4.4.2)

project(pi-thermostat)

include(cmake/dependencies.cmake)
setup_dependencies()


find_package(PkgConfig REQUIRED)

pkg_search_module(gpiod REQUIRED IMPORTED_TARGET libgpiod)
pkg_search_module(gpiodcxx REQUIRED IMPORTED_TARGET libgpiodcxx)

add_subdirectory(pi-thermostat)