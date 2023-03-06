#
# CMake support QP
#
cmake_minimum_required(VERSION 3.14)
project(qp)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
    find_package(check)
else()
    include(FetchContent)
    # Fetch libcheck
    FetchContent_Declare(
        check
        URL https://github.com/libcheck/check/archive/refs/tags/0.15.2.zip
    )
    FetchContent_MakeAvailable(check)
endif()

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

# Add libraries
target_link_libraries(main_test PRIVATE Check::check)

# Add test (single program because nothing more is supported for libcheck)
add_test(NAME main COMMAND main_test)
