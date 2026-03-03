cmake_minimum_required(VERSION 3.7)
project(TransportCatalogue VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_DOCS "Build documentation" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(SOURCES
    transport-catalogue/geo.cpp
    transport-catalogue/json.cpp
    transport-catalogue/json_builder.cpp
    transport-catalogue/json_reader.cpp
    transport-catalogue/main.cpp
    transport-catalogue/map_renderer.cpp
    transport-catalogue/request_handler.cpp
    transport-catalogue/stat_reader.cpp
    transport-catalogue/svg.cpp
    transport-catalogue/transport_catalogue.cpp
    transport-catalogue/transport_router.cpp
)

set(HEADERS
    transport-catalogue/domain.h
    transport-catalogue/geo.h
    transport-catalogue/graph.h
    transport-catalogue/json.h
    transport-catalogue/json_builder.h
    transport-catalogue/json_reader.h
    transport-catalogue/map_renderer.h
    transport-catalogue/ranges.h
    transport-catalogue/request_handler.h
    transport-catalogue/router.h
    transport-catalogue/stat_reader.h
    transport-catalogue/svg.h
    transport-catalogue/transport_catalogue.h
    transport-catalogue/transport_router.h
)

add_executable(transport_catalogue ${SOURCES} ${HEADERS})

target_include_directories(transport_catalogue PRIVATE transport-catalogue)

add_custom_command(TARGET transport_catalogue POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    $<TARGET_FILE_DIR:transport_catalogue>/output
    COMMENT "Creating output directory next to executable"
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/in.json
    ${CMAKE_CURRENT_BINARY_DIR}/in.json
    COPYONLY
)

if(MSVC)
    target_compile_options(transport_catalogue PRIVATE /W4 /WX)
else()
    target_compile_options(transport_catalogue PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic 
        -Werror
        -Wno-unused-parameter
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(transport_catalogue PRIVATE /O2)
    else()
        target_compile_options(transport_catalogue PRIVATE -O3 -DNDEBUG)
    endif()
endif()

if(MSVC)
    target_compile_definitions(transport_catalogue PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Информация о сборке
message(STATUS "========================================")
message(STATUS "Transport Catalogue Configuration:")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:       ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "========================================")
message(STATUS "To build:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake .. -DCMAKE_BUILD_TYPE=Release")
message(STATUS "  cmake --build .")
message(STATUS "")
message(STATUS "To run:")
message(STATUS "  ./transport_catalogue (or transport_catalogue.exe on Windows)")
message(STATUS "========================================")
