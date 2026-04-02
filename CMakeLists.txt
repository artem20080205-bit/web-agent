# cmake_minimum_required(VERSION 3.10)
# project(WEB-AGENT)

# # Указываем компилятору искать заголовочные файлы в текущей директории
# include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# # Добавляем зависимость на nlohmann/json (если используете)
# find_package(nlohmann_json REQUIRED)
# include_directories(lib/inih)

# # Исходные файлы проекта
# set(SOURCES
#     src/agent.cpp
#     lib/inih/ini.c
# )

# add_executable(web_agent ${SOURCES})

# # Связываем с библиотекой nlohmann_json
# target_link_libraries(web_agent nlohmann_json::nlohmann_json)







cmake_minimum_required(VERSION 3.10)
project(WebAgent)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы
set(SRC
    src/agent.cpp
    lib/inih/ini.c  # исходник inih
)

# Добавляем исполняемый файл (объявляем цель)
add_executable(web_agent ${SRC})

# Директории с заголовочными файлами (применяем после объявления цели)
target_include_directories(web_agent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}           # здесь лежит httplib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/inih  # заголовки inih
    ${CMAKE_CURRENT_SOURCE_DIR}/include     # другие заголовки (если есть)
)

# Подключение nlohmann/json
find_package(nlohmann_json REQUIRED)

# Библиотеки для линковки — используем единый ключевой синтаксис с PRIVATE
target_link_libraries(web_agent PRIVATE
    pthread                            # для работы с потоками
    stdc++fs                         # для std::filesystem
    nlohmann_json::nlohmann_json   # библиотека JSON
)

# Компиляционные флаги
target_compile_options(web_agent PRIVATE
    -g                              # отладочная информация
    -Wall                           # все предупреждения
    -Wextra                        # дополнительные предупреждения
    -Werror                       # превращаем предупреждения в ошибки (опционально)
)

# Условная настройка для Debug/Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(web_agent PRIVATE _DEBUG)
endif()

# Копируем конфигурационные файлы в билд‑директорию (опционально)
file(COPY config/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Установка (опционально — для установки программы)
install(TARGETS web_agent DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/${PROJECT_NAME})

# Сообщения о конфигурации
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Binary dir: ${CMAKE_CURRENT_BINARY_DIR}")



