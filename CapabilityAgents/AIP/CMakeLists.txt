cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(AIP LANGUAGES CXX)

include(${AVS_CORE}/build/BuildDefaults.cmake)

add_subdirectory("src")
# Disabling unit test for Windows according to ACSDK-3414
if(NOT WIN32)
    add_subdirectory("test")
endif()
