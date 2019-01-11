cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(DefaultClient VERSION ${PROJECT_VERSION} LANGUAGES CXX)

include(../../build/BuildDefaults.cmake)

add_subdirectory("src")
