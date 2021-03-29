cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(DefaultClient LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("src")
