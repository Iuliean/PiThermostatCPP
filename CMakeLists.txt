cmake_minimum_required(VERSION 4.4.2)

project(pi-thermostat)

include(cmake/dependencies.cmake)
setup_dependencies()


find_package(PkgConfig REQUIRED)

pkg_search_module(gpiod REQUIRED libgpiod)



add_subdirectory(pi-thermostat)