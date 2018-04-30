cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Notifications LANGUAGES CXX)

include(../../build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
