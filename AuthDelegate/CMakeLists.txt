cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(AuthDelegate LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("examples")
add_subdirectory("src")
add_subdirectory("test")
