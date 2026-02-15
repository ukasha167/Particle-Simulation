cmake_minimum_required(VERSION 3.11)
project(main CXX)

# FORCE RELEASE MODE
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# APPLY SYSTEM SPECIFIC FLAGS
if(MSVC) # Visual Studio
    add_compile_options(/O2 /Ot /fp:fast /GL)
    add_link_options(/LTCG)
else() # LINUX / MACOS / MINGW
    add_compile_options(-O3 -ffast-math)

    if(APPLE)
        add_compile_options(-march=native) 
    else()
        # Note: Remove -march=native if you are getting any error
        add_compile_options(-march=native)
    endif()
endif()


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ADDING RAYLIB
include(FetchContent)

set(RAYLIB_VERSION "5.0")
set(RAYLIB_URL "https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz")

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) 
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) 
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    raylib
    URL ${RAYLIB_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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
