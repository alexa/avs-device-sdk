cmake_minimum_required(VERSION 3.0)
project(Alexa LANGUAGES CXX)

include(../../build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
