cmake_minimum_required(VERSION 3.0)
project(acsdkDoNotDisturb LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
