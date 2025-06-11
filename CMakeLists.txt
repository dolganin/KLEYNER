cmake_minimum_required(VERSION 3.16)
project(cleaner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" ON)

if(NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

add_library(core STATIC
    src/scanner.cpp
    src/cleaner.cpp
    src/ui.cpp
    src/platform_win.cpp
    src/platform_nix.cpp
)

target_include_directories(core PUBLIC src)

add_executable(cleaner src/main.cpp)
target_link_libraries(cleaner core)

if(WIN32)
    target_link_libraries(core shell32)
endif()

if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/release-1.12.1.zip
    )
    FetchContent_MakeAvailable(googletest)
    add_executable(tests tests/scanner_tests.cpp)
    target_link_libraries(tests core gtest_main)
    add_test(NAME scanner_tests COMMAND tests)
endif()
