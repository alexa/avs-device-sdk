cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(RegistrationManager LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("src")
acsdk_add_test_subdirectory_if_allowed()