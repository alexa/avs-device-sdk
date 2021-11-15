cmake_minimum_required(VERSION 3.1)
project(acsdkAssetsCommon LANGUAGES CXX)

if (DEFINED AVS_CMAKE_BUILD)
    include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)
endif()

add_subdirectory("src")
add_subdirectory("test")
