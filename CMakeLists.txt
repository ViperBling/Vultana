cmake_minimum_required(VERSION 3.15)

project(Vultana)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DASSETS_DIR=\"${CMAKE_SOURCE_DIR}/Assets/\")

add_subdirectory(External)
add_subdirectory(Framework)
add_subdirectory(Tests)