# CMakeLists.txt для тестов
cmake_minimum_required(VERSION 3.10)
project(WebAgentTests)

# Устанавливаем C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем директорию с исходниками (если нужно)
include_directories(${CMAKE_SOURCE_DIR}/../src)

# Находим все тестовые файлы
file(GLOB TEST_SOURCES 
    "test_*.cpp"
    "*_test.cpp"
)

# Исключаем дубликаты (test_tasks_lifecycle.cpp повторяется)
list(REMOVE_DUPLICATES TEST_SOURCES)

# Создаём исполняемый файл для каждого теста
foreach(TEST_SOURCE ${TEST_SOURCES})
    # Получаем имя теста без расширения
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Создаём исполняемый файл
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # Линкуем необходимые библиотеки (если нужны)
    # target_link_libraries(${TEST_NAME} pthread)
    
    # Добавляем тест в CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # Сообщаем о создании
    message(STATUS "Added test: ${TEST_NAME}")
endforeach()

# Включаем тестирование
enable_testing()

# Опционально: настройка для Google Test (если используется)
# find_package(GTest REQUIRED)
# target_link_libraries(${TEST_NAME} GTest::GTest GTest::Main)