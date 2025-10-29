cmake_minimum_required(VERSION 3.10)

# Set the C++ compiler path if not found automatically.
# Update the path below to the correct location of your g++ compiler.
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

# Project name and language.
project(TME4 LANGUAGES CXX)

# Set the C++ standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none is specified.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set compiler flags.
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -pedantic -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -DNDEBUG -flto")

# Find Qt6
# Update this path to your actual Qt6 installation directory containing Qt6Config.cmake
set(CMAKE_PREFIX_PATH "/opt/Qt/6.5.0/gcc_64/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Gui)

# Add the executable (only main.cpp exists for now).
add_executable(TME4
    src/main.cpp
    src/Tasks.cpp
    src/util/ImageUtils.cpp
    src/util/processRSS.cpp
    src/util/thread_timer.cpp
)

# Specify include directories.
target_include_directories(TME4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link Qt libraries
target_link_libraries(TME4 Qt6::Core Qt6::Gui)

