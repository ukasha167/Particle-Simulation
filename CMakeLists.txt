cmake_minimum_required(VERSION 3.11)
project(main CXX)

# FORCE RELEASE MODE
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# APPLY SYSTEM SPECIFIC FLAGS
if(MSVC)
    # WINDOWS (Visual Studio)
    # /O2   = Maximize speed
    # /Ot   = Favor fast code
    # /fp:fast = Faster floating point math (crucial for physics games)
    # add_compile_options(/O2 /Ot /fp:fast)
    add_compile_options(-O3 -ffast-math -fno-math-errno)
else()
    # LINUX / MACOS / MINGW (GCC or Clang)
    # -O3 = Aggressive optimization
    # -ffast-math = Faster float math (ignores strict IEEE rules for speed)
    add_compile_options(-O3 -ffast-math)

    # Architecture specific optimizations
    if(APPLE)
        # Apple Silicon (M1/M2/M3)
        add_compile_options(-mcpu=apple-m1)
    else()
        # Linux / Windows (Intel/AMD)
        # -march=native = Use all instructions available on THIS machine (AVX2, etc)
        add_compile_options(-march=native)
    endif()
endif()


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ADDING RAYLIB
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# RAYLIB COMES WITH SOME EXAMPLES AND GAMES
# WE DON"T WANT TO BUILD THEM. SO WE TURN THEM OFF
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) 
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) 

# GET RAYLIB
FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raylib)

# SOURCE FILES
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src/")

# EXECUTABLES
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)