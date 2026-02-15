cmake_minimum_required(VERSION 3.14)
project(main CXX)

# FORCE RELEASE MODE
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# APPLY SYSTEM SPECIFIC FLAGS
if(MSVC)
    # WINDOWS (Visual Studio)
    # ---------------------------------------------------------
    # /O2   = Maximize Speed (Equivalent to -O3)
    # /Oi   = Enable Intrinsic Functions (Use CPU instructions directly for math like sqrt, sin, cos)
    # /Ot   = Favor Fast Code (Prioritize speed over size)
    # /GL   = Whole Program Optimization (Allows inlining across different files)
    
    add_compile_options(/O2 /Oi /Ot /GL)
    add_link_options(/LTCG) # Linker option required when using /GL

else()
    # LINUX / MACOS / MINGW (GCC or Clang)
    # ---------------------------------------------------------
    # -O3 = Aggressive optimization
    # -fno-math-errno = Don't set the global 'errno' variable for math (huge speedup for sqrt/pow, 100% safe)
    # -fno-trapping-math = Assume math instructions won't crash the program (safe for games)
    
    add_compile_options(-O3 -fno-math-errno -fno-trapping-math)

    # Architecture specific optimizations
    if(APPLE)
        # Apple Silicon (M-Series)
        add_compile_options(-mcpu=apple-m1)
    else()
        # Linux / Windows (Intel/AMD)
        # -march=native = Use AVX2/AVX512 instructions available on YOUR cpu
        add_compile_options(-march=native)
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ADDING RAYLIB
include(FetchContent)

set(RAYLIB_VERSION "5.5")
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