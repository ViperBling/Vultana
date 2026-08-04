cmake_minimum_required(VERSION 3.20)

# This must run before project() so the vcpkg toolchain can install the
# manifest dependencies during the first configure on a new machine.
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/VultanaVcpkg.cmake)

project(Vultana C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (OutputBinary ${CMAKE_CURRENT_SOURCE_DIR}/Binary)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OutputBinary})

enable_testing()

# file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/Assets DESTINATION ${OutputBinary})

include(${PROJECT_SOURCE_DIR}/Shaders/Shaders.cmake)

add_subdirectory(External)
add_subdirectory(Framework)
add_subdirectory(Tests)