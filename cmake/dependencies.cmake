function(setup_asio)
    include(FetchContent)
    set(FETCHCONTENT_QUIET ON)

    message (STATUS "Setting asio up...")
    FetchContent_Declare(
        asio_source
        URL https://sourceforge.net/projects/asio/files/asio/1.38.2%20%28Stable%29/asio-1.38.2.tar.gz/download
    )
    FetchContent_MakeAvailable(asio_source)

    set(ASIO_INCLUDE_DIR
        "${asio_source_SOURCE_DIR}/include"
        PARENT_SCOPE
    )

    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${asio_source_SOURCE_DIR}/include)
    
    set(FETCHCONTENT_QUIET OFF)
    message (STATUS "Setting asio up...done")

endfunction()

function(setup_crow)
    include(FetchContent)
    set(FETCHCONTENT_QUIET ON)

    message (STATUS "Setting crow up...")
    FetchContent_Declare(
        crow_source
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.3
    )

    FetchContent_MakeAvailable(crow_source)

    add_library(crow INTERFACE)
    target_include_directories(crow INTERFACE ${crow_source_SOURCE_DIR}/include)

    set(FETCHCONTENT_QUIET OFF)
    message (STATUS "Setting crow up...done")

endfunction()

function(setup_libgpiod)
    message (STATUS "Setting libgpiod up...")

    execute_process(
        COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/install_libgpiod.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT_QUIET
        ENVIRONMENT
            MESON_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/toolchain/arm-linux-gnueabihf.ini
            INSTALL_PATH=${CMAKE_INSTALL_PREFIX}
    )

    message(STATUS "Adding libgpod to pkgconfig path")
    set(ENV{PKG_CONFIG_PATH}
        "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
    )

    message (STATUS "Setting libgpiod up...done")

endfunction()

function(setup_dependencies)
    setup_asio()
    setup_crow()
    setup_libgpiod()
endfunction()