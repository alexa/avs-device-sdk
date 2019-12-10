cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Captions LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("Interface")
add_subdirectory("Implementation")