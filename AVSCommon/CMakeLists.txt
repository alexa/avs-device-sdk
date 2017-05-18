cmake_minimum_required(VERSION 3.0)
project(AVSCommon LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
add_subdirectory("AVS/test")
add_subdirectory("Utils/test")



