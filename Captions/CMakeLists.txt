cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Captions LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("Component")
add_subdirectory("Interface")
add_subdirectory("Implementation")