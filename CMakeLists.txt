cmake_minimum_required(VERSION 3.10)
project(CleanerUtility VERSION 1.0 LANGUAGES CXX)

# Используем стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Определяем исполняемый файл и подключаем исходники
add_executable(cleaner
    src/main.cpp
    src/config.cpp
    src/logger.cpp
    src/cleaner.cpp
    src/utils.cpp
)
