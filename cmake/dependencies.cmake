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

    if (CMAKE_CROSSCOMPILING)
        execute_process(
            COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/install_libgpiod.sh
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            OUTPUT_QUIET
            ENVIRONMENT
            MESON_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/toolchain/arm-linux-gnueabihf.ini
            INSTALL_PATH=${CMAKE_INSTALL_PREFIX}
        )
    else()
        message(STATUS "Native build of libgpio")
        execute_process(
            COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/install_libgpiod.sh
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            ENVIRONMENT
            INSTALL_PATH=${CMAKE_INSTALL_PREFIX}
        )
    endif()

    message (STATUS "Setting libgpiod up...done")

endfunction()

function(setup_libi2c)
    message(STATUS "Setting up libi2c-tools...")

    execute_process(
        COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/install_libi2c-tools.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        ENVIRONMENT
            CC=${CMAKE_C_COMPILER}
            CXX=${CMAKE_CXX_COMPILER}
            INSTALL_PATH=${CMAKE_INSTALL_PREFIX}
    )

    add_library(i2c-tools INTERFACE)

    target_include_directories(i2c-tools INTERFACE ${CMAKE_INSTALL_PREFIX}/include)
    target_link_directories(i2c-tools INTERFACE ${CMAKE_INSTALL_PREFIX}/lib)
    target_link_libraries(i2c-tools INTERFACE i2c)

    message(STATUS "Setting up libi2c-tools...done")

endfunction()

function(setup_libcoro)
    message(STATUS "Setting up libcoro...")

    set(LIBCORO_BUILD_TESTS OFF)
    set(LIBCORO_BUILD_EXAMPLES OFF)
    set(LIBCORO_FEATURE_TLS OFF)
    set(LIBCORO_FEATURE_NETWORKING OFF)

    FetchContent_Declare(
        libcoro
        GIT_REPOSITORY https://github.com/jbaldwin/libcoro.git
        GIT_TAG        "v0.16.0"
    )

    FetchContent_MakeAvailable(libcoro)

    
    message(STATUS "Setting up libcoro...done")
endfunction()

function(setup_sqlite3)
    FetchContent_Declare(
        sqlite_source
        GIT_REPOSITORY https://github.com/sqlite/sqlite.git
        GIT_TAG version-3.53.4
    )

    FetchContent_MakeAvailable(sqlite_source)

    execute_process(
        COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/install_sqlite3.sh
        WORKING_DIRECTORY ${sqlite_source_SOURCE_DIR}
        ENVIRONMENT
            CC=${CMAKE_C_COMPILER}
            CXX=${CMAKE_CXX_COMPILER}
            INSTALL_PATH=${CMAKE_INSTALL_PREFIX}
    )

    add_library(sqlite3 INTERFACE)

    target_include_directories(sqlite3 INTERFACE ${CMAKE_INSTALL_PREFIX}/include)
    target_link_directories(sqlite3 INTERFACE ${CMAKE_INSTALL_PREFIX}/lib)
    target_link_libraries(sqlite3 INTERFACE libsqlite3.a)

endfunction()

function(setup_spdlog)
    FetchContent_Declare(
        spdlog_source
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
    )

    FetchContent_MakeAvailable(spdlog_source)
    # execute_process(
    #     COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/install_spdlog.sh
    #     WORKING_DIRECTORY ${spdlog_source_SOURCE_DIR}
    #     ENVIRONMENT
    #         CC=${CMAKE_C_COMPILER}
    #         CXX=${CMAKE_CXX_COMPILER}
    #         INSTALL_PATH=${CMAKE_INSTALL_PREFIX}
    # )

    # add_library(spdlog INTERFACE)
    # target_include_directories(sqlite3 INTERFACE ${CMAKE_INSTALL_PREFIX}/include)
    # target_link_libraries(sqlite3 INTERFACE sqlite3)

endfunction()

function(setup_json)
    include(FetchContent)

    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
    )

    FetchContent_MakeAvailable(json)

endfunction()

function(setup_dependencies)

    set(ENV{PKG_CONFIG_PATH}
        "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}"
    )
    setup_asio()
    setup_crow()
    setup_libgpiod()
    setup_libi2c()
    setup_libcoro()
    setup_sqlite3()
    setup_spdlog()
    setup_json()
endfunction()