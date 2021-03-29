cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(DeviceSettings LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
