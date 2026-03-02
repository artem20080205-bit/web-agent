cmake_minimum_required(VERSION 3.10)
project(WebAgent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_executable(web_agent src/agent.cpp)

target_link_libraries(web_agent ${CURL_LIBRARIES} pthread)
target_include_directories(web_agent PRIVATE ${CURL_INCLUDE_DIRS})