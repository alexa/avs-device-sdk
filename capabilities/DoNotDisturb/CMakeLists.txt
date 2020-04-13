cmake_minimum_required(VERSION 3.0)
project(DoNotDisturbCA LANGUAGES CXX)

include(${AVS_CORE}/build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
