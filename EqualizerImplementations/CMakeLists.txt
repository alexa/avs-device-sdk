cmake_minimum_required(VERSION 3.1)
project(EqualizerImplementations LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
