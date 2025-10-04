cmake_minimum_required(VERSION 3.16)
project(TrafficMask VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки компилятора
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Поиск зависимостей
find_package(Threads REQUIRED)

# Подпапки
add_subdirectory(cpp/core)
add_subdirectory(cpp/signature)
add_subdirectory(cpp/traffic)

# Основная библиотека
add_library(trafficmask_core
    cpp/core/main.cpp
    cpp/core/engine.cpp
    cpp/signature/signature_engine.cpp
    cpp/traffic/traffic_processor.cpp
)

target_link_libraries(trafficmask_core
    Threads::Threads
)

# Установка
install(TARGETS trafficmask_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY cpp/include/ DESTINATION include)
