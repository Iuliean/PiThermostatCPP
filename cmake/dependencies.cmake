function(setup_asio)
    include(FetchContent)

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

endfunction()

function(setup_crow)

    include(FetchContent)

    FetchContent_Declare(
        crow_source
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.3
    )

    FetchContent_MakeAvailable(crow_source)

    add_library(crow INTERFACE)
    target_include_directories(crow INTERFACE ${crow_source_SOURCE_DIR}/include)

endfunction()

function(setup_dependencies)
    setup_asio()
    setup_crow()
endfunction()