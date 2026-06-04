cmake_minimum_required(VERSION 3.14)
project(webagent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(webagent src/main.cpp)
target_include_directories(webagent PRIVATE include)
target_link_libraries(webagent PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(webagent PRIVATE ws2_32)
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(webagent PRIVATE pthread)
endif()

if(MSVC)
    target_compile_options(webagent PRIVATE /W4)
else()
    target_compile_options(webagent PRIVATE -Wall -Wextra)
endif()

install(TARGETS webagent DESTINATION bin)

enable_testing()

add_executable(test_config tests/test_config.cpp)
target_include_directories(test_config PRIVATE include)
target_link_libraries(test_config PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME test_config COMMAND test_config)

add_executable(test_http tests/test_http.cpp)
target_include_directories(test_http PRIVATE include)
target_link_libraries(test_http PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
add_test(NAME test_http COMMAND test_http)

add_executable(test_executor tests/test_executor.cpp)
target_include_directories(test_executor PRIVATE include)
target_link_libraries(test_executor PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME test_executor COMMAND test_executor)
