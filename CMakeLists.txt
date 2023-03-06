#
# CMake support QP
#
cmake_minimum_required(VERSION 3.14)
project(qp)

enable_testing()
include(FetchContent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch libcheck
FetchContent_Declare(
    check
    URL https://github.com/libcheck/check/archive/refs/tags/0.15.2.zip
)
FetchContent_MakeAvailable(check)

# Enable pthread explicitly (fixes libcheck link error on linux)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

# Include current directory
include_directories(AFTER .)

# Main executable
add_executable(
    main_test
    test.c
)
target_link_libraries(main_test PRIVATE Check::check)
