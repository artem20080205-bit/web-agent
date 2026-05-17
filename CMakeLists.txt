cmake_minimum_required(VERSION 3.10)
project(WebAgent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ВКЛЮЧАЕМ SSL ДЛЯ HTTPLIB
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

set(SRC
    src/agent.cpp
    lib/inih/ini.c
)

add_executable(web_agent ${SRC})

target_include_directories(web_agent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/inih
)

find_package(nlohmann_json REQUIRED)

target_link_libraries(web_agent PRIVATE
    pthread
    stdc++fs
    nlohmann_json::nlohmann_json
    ssl
    crypto
)

target_compile_options(web_agent PRIVATE
    -g
    -Wall
    -Wextra
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(web_agent PRIVATE _DEBUG)
endif()

file(COPY config/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    add_subdirectory(tests)
    enable_testing()
endif()