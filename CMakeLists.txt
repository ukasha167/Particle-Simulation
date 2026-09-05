cmake_minimum_required(VERSION 3.25)
project(main CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release mode if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
endif()

# Fetch Raylib
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        6.0
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(raylib)

# Source files & includes
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src/")

# Executable target
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Compiler optimization options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /Oi /Ot)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O3
        -fno-math-errno
        -fno-trapping-math
    )

    if(APPLE)
        # Apple Silicon baseline (runs on all M-series chips)
        target_compile_options(${PROJECT_NAME} PRIVATE -mcpu=apple-m1)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
    endif()
endif()

# Enable Link-Time Optimization (LTO / IPO) if supported by compiler
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "Link-Time Optimization (IPO/LTO) enabled.")
else()
    message(STATUS "IPO/LTO not supported: ${ipo_error}")
endif()
