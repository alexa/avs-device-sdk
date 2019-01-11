cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(ESP VERSION ${PROJECT_VERSION} LANGUAGES CXX C)

include(../build/BuildDefaults.cmake)

add_subdirectory("src")
