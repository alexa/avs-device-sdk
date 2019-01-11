cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(ACL VERSION ${PROJECT_VERSION} LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
