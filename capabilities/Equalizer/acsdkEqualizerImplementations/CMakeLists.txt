cmake_minimum_required(VERSION 3.1)
project(acsdkEqualizerImplementations LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
