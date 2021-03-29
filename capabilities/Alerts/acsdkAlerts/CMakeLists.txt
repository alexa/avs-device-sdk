cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(acsdkAlerts LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
